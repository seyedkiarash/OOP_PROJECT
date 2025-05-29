#include <iostream>
#include <vector>
#include <string>
#include <stdexcept> // For std::invalid_argument and std::runtime_error
#include <map>       // For node and voltage source to index mapping
#include <iomanip>   // For output formatting (std::fixed, std::setprecision)
#include <cmath>     // For std::exp, std::abs in Diode and NR
#include "Eigen/Dense" // Core Eigen library for dense matrices and vectors

// Using namespace std for simplicity in this example
// In larger projects, it's better to use std:: prefix or more limited using declarations.
using namespace std;

// Node class definition
class Node {
private:
    double voltage;
    string name;
    double previousVoltage; // For transient analysis

public:
    Node(const string &name, double voltage = 0.0, double prev_voltage = 0.0) {
        this->name = name;
        this->voltage = voltage;
        this->previousVoltage = prev_voltage;
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

    double getPreviousVoltage() const {
        return previousVoltage;
    }

    void setPreviousVoltage(double pv) {
        previousVoltage = pv;
    }

    void updateVoltageForNextStep() {
        previousVoltage = voltage;
    }

    bool isGround() const {
        return name == "0" || name == "GND" || name == "gnd" || name == "0_rc" || name == "0_diode";
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
    virtual double getValue() const { return 0.0; } // Default, not all elements have a single "value"
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

// Capacitor class
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
};

// *** NEW Diode Class ***
class Diode : public Element {
private:
    double Is;   // Saturation current
    double Vt_n; // Thermal voltage * ideality factor (n*Vt)

public:
    // For Newton-Raphson
    double voltage_k;       // Voltage guess Vd^(k)
    double current_at_voltage_k; // Id(Vd^(k))
    double geq_k;           // Equivalent conductance at Vd^(k)
    double ieq_k;           // Equivalent current source at Vd^(k)

    Diode(Node* n1, Node* n2, const string &name, double saturation_current = 1e-14, double thermal_voltage_n = 0.026)
            : Element(n1, n2, name), Is(saturation_current), Vt_n(thermal_voltage_n),
              voltage_k(0.7), current_at_voltage_k(0.0), geq_k(0.0), ieq_k(0.0) { // Initial guess for Vd
        if (Is <= 0 || Vt_n <=0) {
            throw std::invalid_argument("Diode parameters Is and Vt_n must be positive.");
        }
        updateIterationParameters(voltage_k); // Initialize NR parameters
    }

    string getType() const override { return "Diode"; }

    // Calculates and updates current_at_voltage_k, geq_k, ieq_k based on vd_guess
    void updateIterationParameters(double vd_guess) {
        voltage_k = vd_guess;
        // Shockley diode equation: Id = Is * (exp(Vd / (n*Vt)) - 1)
        double exp_term = std::exp(voltage_k / Vt_n);
        current_at_voltage_k = Is * (exp_term - 1.0);

        // Equivalent conductance: Geq = dId/dVd = (Is / (n*Vt)) * exp(Vd / (n*Vt))
        geq_k = (Is / Vt_n) * exp_term;

        // Equivalent current source: Ieq = Id - Geq * Vd
        ieq_k = current_at_voltage_k - geq_k * voltage_k;
    }

    double getCurrent() const override {
        // This returns the actual current based on final node voltages,
        // not necessarily current_at_voltage_k which is for NR iteration.
        double actual_vd = node1->getVoltage() - node2->getVoltage();
        return Is * (std::exp(actual_vd / Vt_n) - 1.0);
    }
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

    double timeStep_h;

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
    MakingMNA(double h = -1.0) : groundNodeRef(nullptr), timeStep_h(h) {}

    ~MakingMNA() {
        // Memory management of nodes and elements is assumed to be handled outside
    }

    void setTimeStep(double h) {
        if (h <= 0 && elementsInCircuit.end() != std::find_if(elementsInCircuit.begin(), elementsInCircuit.end(), [](Element* e){ return dynamic_cast<Capacitor*>(e) != nullptr || dynamic_cast<Diode*>(e) != nullptr; })) { // Simplified check, diodes might not always need h for DC NR
            throw std::invalid_argument("Error: Time step must be positive for circuits with reactive elements.");
        }
        this->timeStep_h = h;
    }

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
        if(!found) addNode(gnd);
        groundNodeRef = gnd;
    }

    Eigen::MatrixXd getSystemMatrixA() {
        buildNodeAndVoltageSourceMaps();

        int numNonGroundNodes = orderedNonGroundNodes.size();
        int numVoltageSources = orderedVoltageSources.size();
        int systemSize = numNonGroundNodes + numVoltageSources;

        if (systemSize == 0 && numNonGroundNodes == 0) {
            Eigen::MatrixXd A_empty(0,0);
            return A_empty;
        }
        if (systemSize == 0 ) { // Catch cases where systemSize is 0 but numNonGroundNodes might not be if maps are inconsistent
            throw std::runtime_error("Error: Circuit is too small for analysis (e.g. only ground node or inconsistent state).");
        }


        Eigen::MatrixXd A = Eigen::MatrixXd::Zero(systemSize, systemSize);

        // G part (conductances from resistors, capacitors, and diodes)
        for (Element* elem : elementsInCircuit) {
            Node* n1 = elem->getNode1();
            Node* n2 = elem->getNode2();
            double conductance = 0;

            if (auto res = dynamic_cast<Resistor*>(elem)) {
                conductance = 1.0 / res->getValue();
            } else if (auto cap = dynamic_cast<Capacitor*>(elem)) {
                if (this->timeStep_h <= 0) {
                    throw std::runtime_error("Error: Time step h is not set or is invalid for capacitor " + cap->getName() + ". Use setTimeStep().");
                }
                conductance = cap->getValue() / this->timeStep_h; // C/h for Backward Euler
            } else if (auto diode = dynamic_cast<Diode*>(elem)) {
                conductance = diode->geq_k; // Use equivalent conductance from NR iteration
            }

            if (conductance != 0) { // Process if there's a conductive path
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
        }

        // B and C parts (voltage sources)
        for (size_t i = 0; i < orderedVoltageSources.size(); ++i) {
            VoltageSource* vs = orderedVoltageSources[i];
            Node* n_plus = vs->getNode1();
            Node* n_minus = vs->getNode2();
            int vsMNAIndex = vsToIndexMap[vs]; // This index is within the voltage source block

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

        if (systemSize == 0) {
            Eigen::VectorXd Z_empty(0);
            return Z_empty;
        }

        Eigen::VectorXd Z = Eigen::VectorXd::Zero(systemSize);

        // J part (current sources, equivalent current sources from capacitors and diodes)
        for (Element* elem : elementsInCircuit) {
            Node* n1 = elem->getNode1(); // Positive terminal for current source convention
            Node* n2 = elem->getNode2(); // Negative terminal

            if (auto cs = dynamic_cast<CurrentSource*>(elem)) {
                double currentValue = cs->getValue();
                // Current flows from n1 to n2 as per typical definition
                if (n1 != groundNodeRef) { // Current leaving n1
                    Z(nodeToIndexMap[n1]) -= currentValue;
                }
                if (n2 != groundNodeRef) { // Current entering n2
                    Z(nodeToIndexMap[n2]) += currentValue;
                }
            } else if (auto cap = dynamic_cast<Capacitor*>(elem)) {
                if (this->timeStep_h <= 0) {
                    throw std::runtime_error("Error: Time step h is not set or is invalid for capacitor " + cap->getName() + ". Use setTimeStep().");
                }
                double c_div_h = cap->getValue() / this->timeStep_h;
                // V_cap_prev = V_n1_prev - V_n2_prev
                double v_n1_prev = n1->isGround() ? 0.0 : n1->getPreviousVoltage();
                double v_n2_prev = n2->isGround() ? 0.0 : n2->getPreviousVoltage();
                double i_eq_cap = c_div_h * (v_n1_prev - v_n2_prev); // I_eq = (C/h) * V_cap_prev

                // This current source is parallel to C/h, directed from n1 to n2 if V_n1_prev > V_n2_prev
                if (n1 != groundNodeRef) {
                    Z(nodeToIndexMap[n1]) += i_eq_cap;
                }
                if (n2 != groundNodeRef) {
                    Z(nodeToIndexMap[n2]) -= i_eq_cap;
                }
            } else if (auto diode = dynamic_cast<Diode*>(elem)) {
                // Add equivalent current source Ieq = Id_k - Geq_k * Vd_k
                // This current flows from anode (n1) to cathode (n2) in the diode model
                double i_eq_diode = diode->ieq_k;
                if (n1 != groundNodeRef) { // Current source entering n1 (anode)
                    Z(nodeToIndexMap[n1]) += i_eq_diode;
                }
                if (n2 != groundNodeRef) { // Current source leaving n2 (cathode)
                    Z(nodeToIndexMap[n2]) -= i_eq_diode;
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
    const vector<Node*>& getAllNodesInCircuit() const { // Added for convenience
        return allNodesInCircuit;
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
        if (A.rows() == 0 && Z.size() == 0) {
            return Eigen::VectorXd(0);
        }
        if (A.rows() != A.cols() || A.rows() != Z.size()) {
            throw std::runtime_error("Error: Matrix and vector dimensions are not compatible for solving.");
        }
        if (A.rows() == 0) { // Should be caught by the A.rows() == 0 && Z.size() == 0 case or previous errors
            throw std::runtime_error("Error: System of equations is empty but Z is not, or vice-versa.");
        }


        Eigen::PartialPivLU<Eigen::MatrixXd> lu(A);
        if (std::abs(lu.determinant()) < 1e-14) { // Stricter check for singularity
            cout << "Warning: System matrix determinant is very close to zero (" << lu.determinant() << "). Matrix might be singular or ill-conditioned." << endl;
            // Consider throwing an error if critically needed, Eigen might still solve sometimes.
            // throw std::runtime_error("Error: System matrix is singular or ill-conditioned (determinant is near zero).");
        }
        return lu.solve(Z);
    }

    void updateCircuitState(const Eigen::VectorXd& X, MakingMNA& mnaCircuit) {
        const auto& nonGroundNodes = mnaCircuit.getOrderedNonGroundNodes();
        const auto& voltageSources = mnaCircuit.getOrderedVoltageSources();

        int numNonGroundNodes = nonGroundNodes.size();

        if (X.size() == 0 && numNonGroundNodes == 0 && voltageSources.size() == 0) {
            return;
        }

        if (X.size() != numNonGroundNodes + voltageSources.size()) {
            throw std::runtime_error("Error: Solution vector size does not match the number of unknowns.");
        }

        for (int i = 0; i < numNonGroundNodes; ++i) {
            nonGroundNodes[i]->setVoltage(X(i));
        }

        for (size_t i = 0; i < voltageSources.size(); ++i) {
            voltageSources[i]->setCurrent(X(numNonGroundNodes + i));
        }
    }
};

// Main function for testing
int main() {
    cout << fixed << setprecision(8); // Set precision for output

    cout << "--- PDF Section 9: RC Circuit Example (Single Time Step) ---" << endl;

    // Circuit parameters from PDF Section 9.1
    double v_source_val = 5.0;    // V1 = 5V
    double r_val = 1000.0;   // R = 1kOhm
    double c_val = 1.0e-6;   // C = 1uF
    double h_val = 1.0e-6;   // h = 1us (time step)
    double v_cap_initial = 0.0; // Initial capacitor voltage V_1*,n = 0V

    // Define nodes
    Node n_vs_out("N_VS_OUT");
    Node n_cap_top("N_CAP_TOP", v_cap_initial, v_cap_initial); // V_current=0, V_previous=0
    Node n_gnd("0_pdf9"); // This name should be recognized by Node::isGround()

    // Create MNA manager and set time step
    MakingMNA rc_circuit_pdf9(h_val);

    // Add nodes to the circuit
    rc_circuit_pdf9.addNode(&n_vs_out);
    rc_circuit_pdf9.addNode(&n_cap_top);
    rc_circuit_pdf9.addNode(&n_gnd);

    // *** Explicitly set the ground node to ensure it's recognized ***
    rc_circuit_pdf9.setGroundNode(&n_gnd);

    try {
        // Define elements
        VoltageSource vs1(&n_vs_out, &n_gnd, "Vs1", v_source_val);
        Resistor r1(&n_vs_out, &n_cap_top, "R1", r_val);
        Capacitor c1(&n_cap_top, &n_gnd, "C1", c_val);

        // Add elements to the circuit
        rc_circuit_pdf9.addElement(&vs1);
        rc_circuit_pdf9.addElement(&r1);
        rc_circuit_pdf9.addElement(&c1);

        // Get MNA matrices (A and Z)
        Eigen::MatrixXd A_rc9 = rc_circuit_pdf9.getSystemMatrixA();
        Eigen::VectorXd Z_rc9 = rc_circuit_pdf9.getSystemVectorZ();

        cout << "\nSystem Matrix A (for the first step):\n" << A_rc9 << endl;
        cout << "\nSystem Vector Z (for the first step):\n" << Z_rc9 << endl;

        // Solve the system
        MNASolver solver_rc9;
        Eigen::VectorXd X_rc9 = solver_rc9.solve(A_rc9, Z_rc9);

        cout << "\nSolution Vector X (Node Voltages then VS Currents):\n" << X_rc9 << endl;

        // Update node voltages and VS currents from solution X
        solver_rc9.updateCircuitState(X_rc9, rc_circuit_pdf9);

        // Output results
        cout << "\n--- Results after one time step (h = " << h_val << "s) ---" << endl;
        cout << "Node Voltages:" << endl;
        const auto& ordered_nodes = rc_circuit_pdf9.getOrderedNonGroundNodes();
        for (size_t i = 0; i < ordered_nodes.size(); ++i) {
            cout << "Node " << ordered_nodes[i]->getName() << ": " << ordered_nodes[i]->getVoltage() << " V" << endl;
        }

        cout << "\nCurrents through Voltage Sources:" << endl;
        for (const auto* vs_elem : rc_circuit_pdf9.getOrderedVoltageSources()) {
            cout << "Current through " << vs_elem->getName() << ": " << vs_elem->getCurrent() << " A" << endl;
        }

        cout << "\n----------------------------------------------------------" << endl;
        cout << "Capacitor Voltage V(N_CAP_TOP) (V_1*,n+1 from PDF) after 1 step: "
             << n_cap_top.getVoltage() << " V" << endl;
        cout << "Expected Capacitor Voltage from PDF Section 9.3 (approx.): "
             << 0.00499500 << " V (4.995 mV)" << endl;
        cout << "----------------------------------------------------------" << endl;

    } catch (const std::exception& e) {
        cerr << "\nAn error occurred in the PDF Section 9 RC example: " << e.what() << endl;
        return 1;
    }

    return 0;
}