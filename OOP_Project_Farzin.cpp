#include <iostream>
#include <vector>
#include <string>
#include <stdexcept> // For std::invalid_argument and std::runtime_error
#include <map>       // For node and voltage source to index mapping
#include <iomanip>   // For output formatting (std::fixed, std::setprecision)
#include "Eigen/Dense" // Core Eigen library for dense matrices and vectors

// Using namespace std for simplicity in this example
// In larger projects, it's better to use std:: prefix or more limited using declarations.
using namespace std;

// Node class definition
class Node {
private:
    double voltage;
    string name;
    double previousVoltage; // NEW MEMBER: For transient analysis

public:
    // MODIFIED CONSTRUCTOR to include previousVoltage
    Node(const string &name, double voltage = 0.0, double prev_voltage = 0.0) {
        this->name = name;
        this->voltage = voltage;
        this->previousVoltage = prev_voltage; // Initialize previous voltage
    }

    string getName() const {
        return name;
    }

    double getVoltage() const {
        return voltage;
    }

    void setVoltage(double v) {
        voltage = v;
    }

    // NEW METHOD
    double getPreviousVoltage() const {
        return previousVoltage;
    }

    // NEW METHOD
    void setPreviousVoltage(double pv) {
        previousVoltage = pv;
    }

    // Optional: Call after each transient step to update previous voltage for the next step
    void updateVoltageForNextStep() {
        previousVoltage = voltage;
    }

    bool isGround() const {
        return name == "0" || name == "GND" || name == "gnd" || name == "0_rc" /*Added for new example*/;
    }
};

// Element base class definition
class Element {
protected:
    Node *node1, *node2;
    string name;

public:
    Element(Node* n1, Node* n2, const string &name) {
        if (!n1 || !n2) {
            throw std::invalid_argument("Element nodes cannot be null.");
        }
        this->node1 = n1;
        this->node2 = n2;
        this->name = name;
    }

    virtual ~Element() = default;

    string getName() const { return name; }
    Node* getNode1() const { return node1; }
    Node* getNode2() const { return node2; }

    virtual string getType() const = 0;
    virtual double getValue() const = 0;
    virtual double getCurrent() const {
        return 0.0; // Default implementation
    }
    virtual void setCurrent(double current) {
        (void)current; // Default implementation to avoid unused parameter warning
    }
};

// Resistor class
class Resistor : public Element {
private:
    double resistance;

public:
    Resistor(Node* n1, Node* n2, const string &name, double res) : Element(n1, n2, name) {
        if (res <= 0) {
            throw std::invalid_argument("Error: Resistance value must be positive. Resistor: " + name);
        }
        this->resistance = res;
    }

    string getType() const override { return "Resistor"; }
    double getValue() const override { return resistance; }

    double getCurrent() const override {
        if (node1 && node2) {
            return (node1->getVoltage() - node2->getVoltage()) / resistance;
        }
        return 0.0;
    }
};

// VoltageSource class
class VoltageSource : public Element {
private:
    double voltageValue;
    double currentThroughSource;

public:
    VoltageSource(Node* n1, Node* n2, const string &name, double val) : Element(n1, n2, name) {
        this->voltageValue = val;
        this->currentThroughSource = 0.0; // Initial value
    }

    string getType() const override { return "VoltageSource"; }
    double getValue() const override { return voltageValue; }

    double getCurrent() const override {
        return currentThroughSource;
    }
    void setCurrent(double current) override {
        this->currentThroughSource = current;
    }
};

// CurrentSource class
class CurrentSource : public Element {
private:
    double currentValue;

public:
    CurrentSource(Node* n1, Node* n2, const string &name, double val) : Element(n1, n2, name) {
        this->currentValue = val;
    }

    string getType() const override { return "CurrentSource"; }
    double getValue() const override { return currentValue; }
};

// NEW CLASS: Capacitor
class Capacitor : public Element {
private:
    double capacitance;

public:
    Capacitor(Node* n1, Node* n2, const string &name, double cap) : Element(n1, n2, name) {
        if (cap <= 0) {
            throw std::invalid_argument("Error: Capacitance value must be positive. Capacitor: " + name);
        }
        this->capacitance = cap;
    }

    string getType() const override { return "Capacitor"; }
    double getValue() const override { return capacitance; }
    // getCurrent() for a capacitor in DC is 0. For transient MNA, its effect is modeled.
};


// MakingMNA class
class MakingMNA {
private:
    vector<Node*> allNodesInCircuit;
    vector<Element*> elementsInCircuit;
    Node* groundNodeRef;

    map<Node*, int> nodeToIndexMap;
    vector<Node*> orderedNonGroundNodes;
    map<VoltageSource*, int> vsToIndexMap;
    vector<VoltageSource*> orderedVoltageSources;

    double timeStep_h; // NEW MEMBER: For transient analysis

    void buildNodeAndVoltageSourceMaps() {
        nodeToIndexMap.clear();
        orderedNonGroundNodes.clear();
        vsToIndexMap.clear();
        orderedVoltageSources.clear();

        if (!groundNodeRef) {
            for (Node* n : allNodesInCircuit) {
                if (n->isGround()) {
                    groundNodeRef = n;
                    break;
                }
            }
            if (!groundNodeRef) {
                throw std::runtime_error("Error: Ground node not detected in the circuit. Analysis is not possible.");
            }
        }

        int nodeIdx = 0;
        for (Node* node : allNodesInCircuit) {
            if (node != groundNodeRef) {
                orderedNonGroundNodes.push_back(node);
                nodeToIndexMap[node] = nodeIdx++;
            }
        }

        int vsIdx = 0;
        for (Element* elem : elementsInCircuit) {
            if (auto vs = dynamic_cast<VoltageSource*>(elem)) {
                orderedVoltageSources.push_back(vs);
                vsToIndexMap[vs] = vsIdx++;
            }
        }
    }

public:
    // MODIFIED CONSTRUCTOR to accept and store time step h. Defaults to -1 if not for transient.
    MakingMNA(double h = -1.0) : groundNodeRef(nullptr), timeStep_h(h) {}

    ~MakingMNA() {
        // Assuming memory management of nodes and elements is handled outside this class
    }

    // NEW METHOD to set time step if not provided in constructor or to change it
    void setTimeStep(double h) {
        if (h <= 0) {
            throw std::invalid_argument("Error: Time step must be positive.");
        }
        this->timeStep_h = h;
    }

    // NEW METHOD to get time step
    double getTimeStep() const {
        return this->timeStep_h;
    }

    void addNode(Node* node) {
        if (!node) return;
        allNodesInCircuit.push_back(node);
        if (node->isGround()) {
            if (groundNodeRef != nullptr && groundNodeRef != node) {
                cout << "Warning: Multiple ground nodes defined. Using the first identified ground node: "
                     << groundNodeRef->getName() << endl;
            } else if (groundNodeRef == nullptr) {
                groundNodeRef = node;
            }
        }
    }

    void addElement(Element* element) {
        if (!element) return;
        elementsInCircuit.push_back(element);
    }

    void setGroundNode(Node* gnd) {
        if (!gnd) {
            throw std::invalid_argument("Ground node cannot be null.");
        }
        bool found = false;
        for(Node* n : allNodesInCircuit) {
            if(n == gnd) {
                found = true;
                break;
            }
        }
        if(!found) addNode(gnd); // Add ground node if not already in the list

        groundNodeRef = gnd;
    }

    Eigen::MatrixXd getSystemMatrixA() {
        buildNodeAndVoltageSourceMaps();

        int numNonGroundNodes = orderedNonGroundNodes.size();
        int numVoltageSources = orderedVoltageSources.size();
        int systemSize = numNonGroundNodes + numVoltageSources;

        if (systemSize == 0 && numNonGroundNodes == 0) { // MODIFIED CONDITION to allow systemSize=0 if numNonGroundNodes=0
            throw std::runtime_error("Error: Circuit is too small for analysis (no non-ground nodes).");
        }
        if (systemSize == 0 && numNonGroundNodes > 0) { // This case should ideally not happen if buildMaps is correct
            throw std::runtime_error("Error: Circuit is too small for analysis (inconsistent state).");
        }
        if (systemSize == 0) { // If circuit only has ground node or is empty after all
            Eigen::MatrixXd A_empty(0,0); // Return empty matrix for empty system
            return A_empty;
        }


        Eigen::MatrixXd A = Eigen::MatrixXd::Zero(systemSize, systemSize);

        // G part (conductances from resistors and capacitors)
        for (Element* elem : elementsInCircuit) {
            if (auto res = dynamic_cast<Resistor*>(elem)) {
                double conductance = 1.0 / res->getValue();
                Node* n1 = res->getNode1();
                Node* n2 = res->getNode2();

                if (n1 != groundNodeRef) {
                    A(nodeToIndexMap[n1], nodeToIndexMap[n1]) += conductance;
                }
                if (n2 != groundNodeRef) {
                    A(nodeToIndexMap[n2], nodeToIndexMap[n2]) += conductance;
                }
                if (n1 != groundNodeRef && n2 != groundNodeRef) {
                    A(nodeToIndexMap[n1], nodeToIndexMap[n2]) -= conductance;
                    A(nodeToIndexMap[n2], nodeToIndexMap[n1]) -= conductance;
                }
            }
                // NEW BLOCK for Capacitors (Backward Euler: G_eq = C/h)
            else if (auto cap = dynamic_cast<Capacitor*>(elem)) {
                if (this->timeStep_h <= 0) {
                    throw std::runtime_error("Error: Time step h is not set or is invalid for capacitor " + cap->getName() + ". Use setTimeStep().");
                }
                double conductance_eq = cap->getValue() / this->timeStep_h; // C/h
                Node* n1 = cap->getNode1();
                Node* n2 = cap->getNode2();

                if (n1 != groundNodeRef) {
                    A(nodeToIndexMap[n1], nodeToIndexMap[n1]) += conductance_eq;
                }
                if (n2 != groundNodeRef) {
                    A(nodeToIndexMap[n2], nodeToIndexMap[n2]) += conductance_eq;
                }
                if (n1 != groundNodeRef && n2 != groundNodeRef) {
                    A(nodeToIndexMap[n1], nodeToIndexMap[n2]) -= conductance_eq;
                    A(nodeToIndexMap[n2], nodeToIndexMap[n1]) -= conductance_eq;
                }
            }
        }

        // B and C parts (voltage sources)
        for (size_t i = 0; i < orderedVoltageSources.size(); ++i) {
            VoltageSource* vs = orderedVoltageSources[i];
            Node* n_plus = vs->getNode1();
            Node* n_minus = vs->getNode2();
            int vsMNAIndex = vsToIndexMap[vs];

            if (n_plus != groundNodeRef) {
                int nodeIdx = nodeToIndexMap[n_plus];
                A(nodeIdx, numNonGroundNodes + vsMNAIndex) += 1.0;
                A(numNonGroundNodes + vsMNAIndex, nodeIdx) += 1.0;
            }
            if (n_minus != groundNodeRef) {
                int nodeIdx = nodeToIndexMap[n_minus];
                A(nodeIdx, numNonGroundNodes + vsMNAIndex) -= 1.0;
                A(numNonGroundNodes + vsMNAIndex, nodeIdx) -= 1.0;
            }
        }
        return A;
    }

    Eigen::VectorXd getSystemVectorZ() {
        // buildNodeAndVoltageSourceMaps() should have been called by getSystemMatrixA
        int numNonGroundNodes = orderedNonGroundNodes.size();
        int numVoltageSources = orderedVoltageSources.size();
        int systemSize = numNonGroundNodes + numVoltageSources;

        if (systemSize == 0) { // MODIFIED: handle empty system consistently
            Eigen::VectorXd Z_empty(0);
            return Z_empty;
        }

        Eigen::VectorXd Z = Eigen::VectorXd::Zero(systemSize);

        // J part (current sources and equivalent current sources from capacitors)
        for (Element* elem : elementsInCircuit) {
            if (auto cs = dynamic_cast<CurrentSource*>(elem)) {
                Node* n_from = cs->getNode1();
                Node* n_to = cs->getNode2();
                double currentValue = cs->getValue();

                if (n_to != groundNodeRef) {
                    Z(nodeToIndexMap[n_to]) += currentValue;
                }
                if (n_from != groundNodeRef) {
                    Z(nodeToIndexMap[n_from]) -= currentValue;
                }
            }
                // NEW BLOCK for Capacitors (Backward Euler: I_eq = (C/h) * V_cap_prev)
            else if (auto cap = dynamic_cast<Capacitor*>(elem)) {
                if (this->timeStep_h <= 0) {
                    throw std::runtime_error("Error: Time step h is not set or is invalid for capacitor " + cap->getName() + ". Use setTimeStep().");
                }
                Node* n1 = cap->getNode1();
                Node* n2 = cap->getNode2();
                double c_div_h = cap->getValue() / this->timeStep_h;

                // V_cap_prev = V_n1_prev - V_n2_prev
                // I_eq = (C/h) * V_cap_prev, flows into n1 from n2 perspective in model
                double v_n1_prev = n1->isGround() ? 0.0 : n1->getPreviousVoltage();
                double v_n2_prev = n2->isGround() ? 0.0 : n2->getPreviousVoltage();
                double i_eq = c_div_h * (v_n1_prev - v_n2_prev);

                if (n1 != groundNodeRef) {
                    Z(nodeToIndexMap[n1]) += i_eq;
                }
                if (n2 != groundNodeRef) {
                    Z(nodeToIndexMap[n2]) -= i_eq; // Current flows out of n2, consistent with I_eq into n1
                }
            }
        }

        // E part (voltage source values)
        for (size_t i = 0; i < orderedVoltageSources.size(); ++i) {
            VoltageSource* vs = orderedVoltageSources[i];
            int vsMNAIndex = vsToIndexMap[vs];
            Z(numNonGroundNodes + vsMNAIndex) = vs->getValue();
        }
        return Z;
    }

    const vector<Node*>& getOrderedNonGroundNodes() const {
        return orderedNonGroundNodes;
    }

    const vector<VoltageSource*>& getOrderedVoltageSources() const {
        return orderedVoltageSources;
    }
    const map<VoltageSource*, int>& getVoltageSourceToIndexMap() const {
        return vsToIndexMap;
    }
    const vector<Element*>& getAllElements() const {
        return elementsInCircuit;
    }
};

// MNASolver class
class MNASolver {
public:
    MNASolver() {}

    Eigen::VectorXd solve(const Eigen::MatrixXd& A, const Eigen::VectorXd& Z) {
        if (A.rows() == 0 && Z.size() == 0) { // MODIFIED: Handle empty system
            // For an empty system, the solution is also empty.
            return Eigen::VectorXd(0);
        }
        if (A.rows() != A.cols() || A.rows() != Z.size()) {
            throw std::runtime_error("Error: Matrix and vector dimensions are not compatible for solving.");
        }
        if (A.rows() == 0) { // Should be caught by the A.rows() == 0 && Z.size() == 0 case or previous errors
            throw std::runtime_error("Error: System of equations is empty but Z is not, or vice-versa.");
        }


        Eigen::PartialPivLU<Eigen::MatrixXd> lu(A);
        if (std::abs(lu.determinant()) < 1e-12) { // MODIFIED LINE: Slightly more robust check for singularity
            cout << "Warning: System matrix determinant is very close to zero (" << lu.determinant() << "). Matrix might be singular or ill-conditioned." << endl;
            // Depending on the Eigen version and matrix properties, solve() might still proceed or throw.
            // For critical applications, a more sophisticated check or SVD might be needed.
            // Throwing an error here prevents potential crashes or NaN results from Eigen's solve() with singular matrices.
            throw std::runtime_error("Error: System matrix is singular or ill-conditioned (determinant is near zero). The circuit may not be solvable (e.g., floating sections, redundant voltage sources).");
        }
        return lu.solve(Z);
    }

    void updateCircuitState(const Eigen::VectorXd& X, MakingMNA& mnaCircuit) {
        const auto& nonGroundNodes = mnaCircuit.getOrderedNonGroundNodes();
        const auto& voltageSources = mnaCircuit.getOrderedVoltageSources();
        // const auto& vsMap = mnaCircuit.getVoltageSourceToIndexMap(); // vsMap not directly used here but good for consistency

        int numNonGroundNodes = nonGroundNodes.size();

        if (X.size() == 0 && numNonGroundNodes == 0 && voltageSources.size() == 0) { // MODIFIED: Handle empty solution
            return; // Nothing to update for an empty circuit
        }

        if (X.size() != numNonGroundNodes + voltageSources.size()) {
            throw std::runtime_error("Error: Solution vector size does not match the number of unknowns.");
        }

        // Update node voltages
        for (int i = 0; i < numNonGroundNodes; ++i) {
            nonGroundNodes[i]->setVoltage(X(i));
            // For multi-step transient, you would call:
            // nonGroundNodes[i]->updateVoltageForNextStep(); // or do this in the simulation loop
        }

        // Update currents through voltage sources
        // The vsMap is implicitly used by orderedVoltageSources matching the latter part of X
        for (size_t i = 0; i < voltageSources.size(); ++i) {
            // The index in X for the i-th voltage source in orderedVoltageSources is numNonGroundNodes + i
            // This relies on buildNodeAndVoltageSourceMaps correctly ordering vsToIndexMap implicitly
            // and orderedVoltageSources being the source for that ordering.
            voltageSources[i]->setCurrent(X(numNonGroundNodes + i));
        }
    }
};

// Main function for testing
int main() {
    // Set output precision for floating-point numbers
    cout << fixed << setprecision(6);

    // --- ORIGINAL DC EXAMPLE (Resistor-Resistor circuit) ---
    cout << "--- Original DC Example (R-R Circuit) ---" << endl;
    Node n1("1"), n2("2"), n_gnd("0");

    MakingMNA circuit_dc; // Uses default constructor, timeStep_h is -1
    circuit_dc.addNode(&n1);
    circuit_dc.addNode(&n2);
    circuit_dc.addNode(&n_gnd);
    // circuit_dc.setGroundNode(&n_gnd); // Auto-detected

    try {
        VoltageSource vs(&n1, &n_gnd, "V1", 5.0);
        Resistor r1(&n1, &n2, "R1", 1000.0);
        Resistor r2(&n2, &n_gnd, "R2", 2000.0);

        circuit_dc.addElement(&vs);
        circuit_dc.addElement(&r1);
        circuit_dc.addElement(&r2);

        Eigen::MatrixXd A_dc = circuit_dc.getSystemMatrixA();
        Eigen::VectorXd Z_dc = circuit_dc.getSystemVectorZ();

        cout << "System Matrix A (DC):\n" << A_dc << endl << endl;
        cout << "System Vector Z (DC):\n" << Z_dc << endl << endl;

        MNASolver solver_dc;
        Eigen::VectorXd X_dc = solver_dc.solve(A_dc, Z_dc);

        cout << "Solution Vector X (DC):\n" << X_dc << endl << endl;

        solver_dc.updateCircuitState(X_dc, circuit_dc);

        cout << "Node Voltages after solving (DC):" << endl;
        for (const auto* node : circuit_dc.getOrderedNonGroundNodes()) {
            cout << "Node " << node->getName() << ": " << node->getVoltage() << " V" << endl;
        }
        cout << "\nCurrents through Voltage Sources (DC):" << endl;
        for (const auto* vs_elem : circuit_dc.getOrderedVoltageSources()) {
            cout << "Current through " << vs_elem->getName() << ": " << vs_elem->getCurrent() << " A" << endl;
        }
        cout << "\nCurrents through Resistors (DC - calculated after solving):" << endl;
        for (const auto* elem : circuit_dc.getAllElements()) {
            if (const Resistor* res = dynamic_cast<const Resistor*>(elem)) {
                cout << "Current through " << res->getName() << " (" << res->getNode1()->getName() << "->" << res->getNode2()->getName() << "): "
                     << res->getCurrent() << " A" << endl;
            }
        }
    } catch (const std::exception& e) {
        cerr << "An error occurred in DC example: " << e.what() << endl;
    }

    cout << "\n\n--- RC Circuit Example (Transient - 1 step Backward Euler) ---" << endl;
    // Set output precision for finer comparison if needed for RC example
    cout << fixed << setprecision(8);

    // Nodes for RC example (Vs - N_S_OUT - R - N_CAP_TOP - C - GND_RC)
    // Initial voltage for N_CAP_TOP is 0 (Vc_prev = 0 as per PDF example V_1*,n = 0V)
    Node n_s_out_rc("N_S_OUT_RC");
    Node n_cap_top_rc("N_CAP_TOP_RC", 0.0, 0.0); // Initial voltage = 0, previousVoltage (V_n) = 0.0
    Node n_gnd_rc("0_rc");                      // Ground node for RC circuit

    // Circuit manager with time step h = 1us = 1.0e-6 s
    double time_step_h = 1.0e-6;
    MakingMNA rc_circuit(time_step_h); // Pass h to constructor
    // Alternatively:
    // MakingMNA rc_circuit;
    // rc_circuit.setTimeStep(time_step_h);

    rc_circuit.addNode(&n_s_out_rc);
    rc_circuit.addNode(&n_cap_top_rc);
    rc_circuit.addNode(&n_gnd_rc); // Will be auto-detected as ground due to its name "0_rc" (added to isGround check)
    // or explicitly: rc_circuit.setGroundNode(&n_gnd_rc);

    try {
        // Elements for RC circuit (V1=5V, R=1k, C=1uF)
        VoltageSource vs_rc(&n_s_out_rc, &n_gnd_rc, "Vs_RC", 5.0);   // 5V voltage source
        Resistor r_rc(&n_s_out_rc, &n_cap_top_rc, "R_RC", 1000.0);    // 1 kOhm resistor
        Capacitor c_rc(&n_cap_top_rc, &n_gnd_rc, "C_RC", 1.0e-6);    // 1 uF capacitor

        rc_circuit.addElement(&vs_rc);
        rc_circuit.addElement(&r_rc);
        rc_circuit.addElement(&c_rc);

        // Get MNA matrices for RC circuit
        Eigen::MatrixXd A_rc = rc_circuit.getSystemMatrixA();
        Eigen::VectorXd Z_rc = rc_circuit.getSystemVectorZ();

        cout << "RC Circuit System Matrix A:\n" << A_rc << endl << endl;
        cout << "RC Circuit System Vector Z:\n" << Z_rc << endl << endl;

        // Solve the system for RC circuit
        MNASolver solver_rc;
        Eigen::VectorXd X_rc = solver_rc.solve(A_rc, Z_rc);

        cout << "RC Circuit Solution Vector X (Node Voltages then VS Currents):\n" << X_rc << endl << endl;

        // Update node voltages and VS currents
        solver_rc.updateCircuitState(X_rc, rc_circuit);

        // Print results for RC circuit
        cout << "RC Circuit Node Voltages after solving (t = h):" << endl;
        for (const auto* node : rc_circuit.getOrderedNonGroundNodes()) {
            cout << "Node " << node->getName() << ": " << node->getVoltage() << " V";
            if (node->getName() == "N_CAP_TOP_RC") {
                cout << " (Capacitor voltage. Expected ~0.00499500 V from PDF example calculation)";
            }
            cout << endl;
        }

        cout << "\nRC Circuit Currents through Voltage Sources:" << endl;
        for (const auto* vs_elem : rc_circuit.getOrderedVoltageSources()) {
            cout << "Current through " << vs_elem->getName() << ": " << vs_elem->getCurrent() << " A" << endl;
        }

        cout << "\nRC Circuit Currents through Resistors (calculated after solving):" << endl;
        for (const auto* elem : rc_circuit.getAllElements()) {
            if (const Resistor* res = dynamic_cast<const Resistor*>(elem)) {
                cout << "Current through " << res->getName() << " (" << res->getNode1()->getName() << "->" << res->getNode2()->getName() << "): "
                     << res->getCurrent() << " A" << endl;
            }
        }
        // Note: Current through capacitor is implicitly handled by the MNA formulation.

    } catch (const std::exception& e) {
        cerr << "An error occurred in RC example: " << e.what() << endl;
    }


    return 0;
}