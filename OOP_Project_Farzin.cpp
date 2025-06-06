#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <map>
#include <cmath>
#include <algorithm>
#include "Eigen/Dense" // Core Eigen library for dense matrices and vectors

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
        return name == "0" || name == "GND" || name == "gnd" ||
               name == "0_rc" || name == "0_diode" ||
               name == "0_pdf9" || name == "0_pdf10"; // Added new ground names from examples
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
            throw std::invalid_argument("Element nodes cannot be null for element: " + name);
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

// Inductor Class
class Inductor : public Element {
private:
    double inductance;
    double current;         // Current I_L at t_n+1
    double previousCurrent; // Current I_L at t_n

public:
    Inductor(Node* n1, Node* n2, const string &name, double ind)
            : Element(n1, n2, name), inductance(ind), current(0.0), previousCurrent(0.0) {
        if (ind <= 0) {
            throw std::invalid_argument("Error: Inductance value must be positive. Inductor: " + name);
        }
    }

    string getType() const override { return "Inductor"; }
    double getValue() const override { return inductance; } // Returns L

    double getCurrent() const override { return current; }
    void setCurrent(double c) { current = c; }

    double getPreviousCurrent() const { return previousCurrent; }
    void setPreviousCurrent(double pc) { previousCurrent = pc; }

    // Method to update state for next time step (call after each successful time step solution)
    void updateCurrentForNextStep() {
        previousCurrent = current;
    }
};

// Diode Class
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
            throw std::invalid_argument("Diode parameters Is and Vt_n must be positive for Diode: " + name);
        }
        updateIterationParameters(voltage_k); // Initialize NR parameters
    }

    string getType() const override { return "Diode"; }

    void updateIterationParameters(double vd_guess) {
        voltage_k = vd_guess;
        double exp_term = std::exp(voltage_k / Vt_n);
        current_at_voltage_k = Is * (exp_term - 1.0);
        geq_k = (Is / Vt_n) * exp_term;
        ieq_k = current_at_voltage_k - geq_k * voltage_k;
    }

    double getCurrent() const override {
        double actual_vd = node1->getVoltage() - node2->getVoltage();
        return Is * (std::exp(actual_vd / Vt_n) - 1.0);
    }

    double getThermalVoltageN() const { return Vt_n; }
};


// MakingMNA class
class MakingMNA {
private:
    vector<Node *> allNodesInCircuit;
    vector<Element *> elementsInCircuit;
    Node *groundNodeRef;

    map<Node *, int> nodeToIndexMap;
    vector<Node *> orderedNonGroundNodes;
    map<VoltageSource *, int> vsToIndexMap;
    vector<VoltageSource *> orderedVoltageSources;
    map<Inductor *, int> inductorToIndexMap;
    vector<Inductor *> orderedInductors;
    vector<Diode *> orderedDiodes;

    double timeStep_h;

    void buildSystemMaps() {
        nodeToIndexMap.clear();
        orderedNonGroundNodes.clear();
        vsToIndexMap.clear();
        orderedVoltageSources.clear();
        inductorToIndexMap.clear();
        orderedInductors.clear();
        orderedDiodes.clear();

        if (!groundNodeRef) {
            for (Node *n: allNodesInCircuit) {
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
        for (Node *node: allNodesInCircuit) {
            if (node != groundNodeRef) {
                orderedNonGroundNodes.push_back(node);
                nodeToIndexMap[node] = nodeIdx++;
            }
        }

        int vsIdx = 0;
        for (Element *elem: elementsInCircuit) {
            if (auto vs = dynamic_cast<VoltageSource *>(elem)) {
                orderedVoltageSources.push_back(vs);
                vsToIndexMap[vs] = vsIdx++;
            }
        }

        int indIdx = 0;
        for (Element *elem: elementsInCircuit) {
            if (auto ind = dynamic_cast<Inductor *>(elem)) {
                orderedInductors.push_back(ind);
                inductorToIndexMap[ind] = indIdx++;
            }
        }

        for ( Element *elem: elementsInCircuit) {
            if (auto d = dynamic_cast<Diode *>(elem)) {
                orderedDiodes.push_back(d);
            }
        }
    }

public:
    MakingMNA(double h = -1.0) : groundNodeRef(nullptr), timeStep_h(h) {}

    ~MakingMNA() {
        // Memory management of nodes and elements is assumed to be handled outside
    }

    void setTimeStep(double h) {
        // Simplified check, more robust checks are inside getSystemMatrixA/Z for specific elements
        if (h <= 0 && std::any_of(elementsInCircuit.begin(), elementsInCircuit.end(), [](Element* e){
            return dynamic_cast<Capacitor*>(e) != nullptr || dynamic_cast<Inductor*>(e) != nullptr;
        })) {
            // This is a general warning/check. Specific checks are better placed when an element requiring h is processed.
            cout << "Warning: Setting non-positive time step for a circuit that might contain reactive elements." << endl;
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
        buildSystemMaps();

        int numNonGroundNodes = orderedNonGroundNodes.size();
        int numVoltageSources = orderedVoltageSources.size();
        int numInductors = orderedInductors.size();
        int systemSize = numNonGroundNodes + numVoltageSources + numInductors;

        if (systemSize == 0) { // Simplified check: if system size is 0, it's empty
            Eigen::MatrixXd A_empty(0,0);
            return A_empty;
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
                conductance = cap->getValue() / this->timeStep_h;
            } else if (auto diode = dynamic_cast<Diode*>(elem)) {
                conductance = diode->geq_k;
            }

            if (conductance != 0) {
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

        // Stamps for Voltage Sources
        for (size_t i = 0; i < orderedVoltageSources.size(); ++i) {
            VoltageSource* vs = orderedVoltageSources[i];
            Node* n_plus = vs->getNode1();
            Node* n_minus = vs->getNode2();
            int vsCurrentVarIndex = numNonGroundNodes + vsToIndexMap[vs];
            int vsBranchEqRow = numNonGroundNodes + vsToIndexMap[vs];

            if (n_plus != groundNodeRef) {
                A(nodeToIndexMap[n_plus], vsCurrentVarIndex) += 1.0; // KCL at n_plus: +I_vs
                A(vsBranchEqRow, nodeToIndexMap[n_plus]) += 1.0;     // Branch Eq: +V_nplus
            }
            if (n_minus != groundNodeRef) {
                A(nodeToIndexMap[n_minus], vsCurrentVarIndex) -= 1.0; // KCL at n_minus: -I_vs
                A(vsBranchEqRow, nodeToIndexMap[n_minus]) -= 1.0;     // Branch Eq: -V_nminus
            }
            // A(vsBranchEqRow, vsCurrentVarIndex) is 0 for ideal VS (D matrix part)
        }

        // Stamps for Inductors
        for (size_t i = 0; i < orderedInductors.size(); ++i) {
            Inductor* ind = orderedInductors[i];
            Node* n1_ind = ind->getNode1();
            Node* n2_ind = ind->getNode2();
            int indCurrentVarIndex = numNonGroundNodes + numVoltageSources + inductorToIndexMap[ind];
            int indBranchEqRow = numNonGroundNodes + numVoltageSources + inductorToIndexMap[ind];

            if (this->timeStep_h <= 0) {
                throw std::runtime_error("Error: Time step h is not set or is invalid for inductor " + ind->getName() + ". Use setTimeStep().");
            }
            double l_div_h = ind->getValue() / this->timeStep_h;

            if (n1_ind != groundNodeRef) {
                A(nodeToIndexMap[n1_ind], indCurrentVarIndex) += 1.0; // KCL at n1_ind: +I_L
                A(indBranchEqRow, nodeToIndexMap[n1_ind]) += 1.0;     // Branch Eq: +V_n1_ind
            }
            if (n2_ind != groundNodeRef) {
                A(nodeToIndexMap[n2_ind], indCurrentVarIndex) -= 1.0; // KCL at n2_ind: -I_L
                A(indBranchEqRow, nodeToIndexMap[n2_ind]) -= 1.0;     // Branch Eq: -V_n2_ind
            }
            A(indBranchEqRow, indCurrentVarIndex) -= l_div_h;         // Branch Eq: -(L/h)I_L
        }
        return A;
    }

    Eigen::VectorXd getSystemVectorZ() {
        // Ensure maps are built if this is called independently, though A usually builds them.
        if (nodeToIndexMap.empty() && !allNodesInCircuit.empty() && groundNodeRef && !orderedNonGroundNodes.empty()) {
            // Potentially stale maps, or A was not called.
        } else if (nodeToIndexMap.empty() && !allNodesInCircuit.empty() && groundNodeRef) {
            buildSystemMaps(); // Build if completely uninitialized
        }


        int numNonGroundNodes = orderedNonGroundNodes.size();
        int numVoltageSources = orderedVoltageSources.size();
        int numInductors = orderedInductors.size();
        int systemSize = numNonGroundNodes + numVoltageSources + numInductors;

        if (systemSize == 0) {
            Eigen::VectorXd Z_empty(0);
            return Z_empty;
        }

        Eigen::VectorXd Z = Eigen::VectorXd::Zero(systemSize);

        // J part (current sources, equivalent Cs from capacitors and diodes)
        for (Element* elem : elementsInCircuit) {
            Node* n1 = elem->getNode1();
            Node* n2 = elem->getNode2();

            if (auto cs = dynamic_cast<CurrentSource*>(elem)) {
                double currentValue = cs->getValue();
                if (n1 != groundNodeRef) {
                    Z(nodeToIndexMap[n1]) -= currentValue;
                }
                if (n2 != groundNodeRef) {
                    Z(nodeToIndexMap[n2]) += currentValue;
                }
            } else if (auto cap = dynamic_cast<Capacitor*>(elem)) {
                if (this->timeStep_h <= 0) {
                    throw std::runtime_error("Error: Time step h is not set or is invalid for capacitor " + cap->getName() + ". Use setTimeStep().");
                }
                double c_div_h = cap->getValue() / this->timeStep_h;
                double v_n1_prev = n1->isGround() ? 0.0 : n1->getPreviousVoltage();
                double v_n2_prev = n2->isGround() ? 0.0 : n2->getPreviousVoltage();
                double i_eq_cap = c_div_h * (v_n1_prev - v_n2_prev);

                if (n1 != groundNodeRef) {
                    Z(nodeToIndexMap[n1]) += i_eq_cap;
                }
                if (n2 != groundNodeRef) {
                    Z(nodeToIndexMap[n2]) -= i_eq_cap;
                }
            } else if (auto diode = dynamic_cast<Diode*>(elem)) {
                double i_eq_diode = diode->ieq_k;
                if (n1 != groundNodeRef) {
                    Z(nodeToIndexMap[n1]) += i_eq_diode;
                }
                if (n2 != groundNodeRef) {
                    Z(nodeToIndexMap[n2]) -= i_eq_diode;
                }
            }
        }

        // E part (voltage source values on RHS of their branch equations)
        for (size_t i = 0; i < orderedVoltageSources.size(); ++i) {
            VoltageSource* vs = orderedVoltageSources[i];
            int vsBranchEqRow = numNonGroundNodes + vsToIndexMap[vs];
            Z(vsBranchEqRow) = vs->getValue();
        }

        // RHS for Inductor branch equations
        // Branch eq: V(n1) - V(n2) - (L/h)I_L,n+1 = -(L/h)I_L,n
        for (size_t i = 0; i < orderedInductors.size(); ++i) {
            Inductor* ind = orderedInductors[i];
            int indBranchEqRow = numNonGroundNodes + numVoltageSources + inductorToIndexMap[ind];

            if (this->timeStep_h <= 0) {
                throw std::runtime_error("Error: Time step h is not set or is invalid for inductor " + ind->getName() + ". Use setTimeStep().");
            }
            double l_div_h = ind->getValue() / this->timeStep_h;
            Z(indBranchEqRow) = -(l_div_h * ind->getPreviousCurrent());
        }
        return Z;
    }

    const vector<Node*>& getOrderedNonGroundNodes() const {
        return orderedNonGroundNodes;
    }
    const vector<Node*>& getAllNodesInCircuit() const {
        return allNodesInCircuit;
    }

    const vector<VoltageSource*>& getOrderedVoltageSources() const {
        return orderedVoltageSources;
    }
    const vector<Inductor*>& getOrderedInductors() const {
        return orderedInductors;
    }
    const map<VoltageSource*, int>& getVoltageSourceToIndexMap() const {
        return vsToIndexMap;
    }
    const vector<Element*>& getAllElements() const {
        return elementsInCircuit;
    }
    const vector<Diode*>& getOrderedDiodes() const {
        return orderedDiodes;
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
            throw std::runtime_error("Error: Matrix and vector dimensions are not compatible for solving. A: " +
                                     to_string(A.rows()) + "x" + to_string(A.cols()) + ", Z: " + to_string(Z.size()));
        }
        if (A.rows() == 0) {
            throw std::runtime_error("Error: System of equations is empty (A has 0 rows).");
        }

        Eigen::PartialPivLU<Eigen::MatrixXd> lu(A);
        if (A.rows() > 0 && std::abs(lu.determinant()) < 1e-14) {
            cout << "Warning: System matrix determinant is very close to zero (" << lu.determinant() << "). Matrix might be singular or ill-conditioned." << endl;
        }
        return lu.solve(Z);
    }

    void updateCircuitState(const Eigen::VectorXd& X, MakingMNA& mnaCircuit) {
        const auto& nonGroundNodes = mnaCircuit.getOrderedNonGroundNodes();
        const auto& voltageSources = mnaCircuit.getOrderedVoltageSources();
        const auto& inductors = mnaCircuit.getOrderedInductors(); // New

        int numNonGroundNodes = nonGroundNodes.size();
        int numVoltageSources = voltageSources.size();
        int numInductors = inductors.size();

        if (X.size() == 0 && numNonGroundNodes == 0 && numVoltageSources == 0 && numInductors == 0) {
            return; // Nothing to update for an empty solved system
        }

        if (static_cast<size_t>(X.size()) != numNonGroundNodes + numVoltageSources + numInductors) {
            throw std::runtime_error("Error: Solution vector size (" + to_string(X.size()) +
                                     ") does not match the number of unknowns (" +
                                     to_string(numNonGroundNodes + numVoltageSources + numInductors) + ").");
        }

        for (int i = 0; i < numNonGroundNodes; ++i) {
            nonGroundNodes[i]->setVoltage(X(i));
        }

        for (size_t i = 0; i < voltageSources.size(); ++i) {
            voltageSources[i]->setCurrent(X(numNonGroundNodes + i));
        }

        for (size_t i = 0; i < inductors.size(); ++i) {
            inductors[i]->setCurrent(X(numNonGroundNodes + numVoltageSources + i));
        }
    }
};

// ===================================================================================
// ===== NEW CODE FOR ADAPTIVE TIME-STEPPING STARTS HERE =============================
// ===================================================================================

class TransientAnalysis {
private:
    MakingMNA& mnaCircuit;
    MNASolver solver;

    // Parameters for adaptive time-stepping
    double initial_h;
    double min_h;
    double max_h;
    double tolerance;
    double h_increase_factor;
    double h_decrease_factor;

public:
    TransientAnalysis(MakingMNA& circuit, double initial_step, double min_step, double max_step, double tol)
            : mnaCircuit(circuit), solver(), initial_h(initial_step), min_h(min_step),
              max_h(max_step), tolerance(tol), h_increase_factor(1.5), h_decrease_factor(2.0) {
        if (min_h <= 0 || max_h <= min_h || initial_h < min_h || initial_h > max_h) {
            throw std::invalid_argument("Invalid time step parameters for adaptive analysis.");
        }
    }

    void run(double t_stop) {
        double current_time = 0.0;
        double h = initial_h;

        // --- Print Header ---
        cout << "Time (s)\t";
        const auto& nonGroundNodes = mnaCircuit.getOrderedNonGroundNodes();
        for (const auto& node : nonGroundNodes) {
            cout << "V(" << node->getName() << ")\t";
        }
        cout << "(h)" << endl;
        cout << "--------------------------------------------------------" << endl;


        while (current_time < t_stop) {
            // Adjust h if it would overshoot t_stop
            if (current_time + h > t_stop) {
                h = t_stop - current_time;
            }

            // Store current voltages to estimate error after the trial step
            std::vector<double> old_voltages;
            for(const auto& node : nonGroundNodes) {
                old_voltages.push_back(node->getVoltage());
            }

            bool step_accepted = false;
            while (!step_accepted) {
                // Prevent h from going below the minimum allowed step
                if (h < min_h) {
                    h = min_h;
                }

                mnaCircuit.setTimeStep(h);
                Eigen::MatrixXd A = mnaCircuit.getSystemMatrixA();
                Eigen::VectorXd Z = mnaCircuit.getSystemVectorZ();
                Eigen::VectorXd X = solver.solve(A, Z);

                // Simple error estimation based on max voltage change
                double max_voltage_change = 0.0;
                for (size_t i = 0; i < nonGroundNodes.size(); ++i) {
                    double change = std::abs(X(i) - old_voltages[i]);
                    if (change > max_voltage_change) {
                        max_voltage_change = change;
                    }
                }

                // Accept the step if error is within tolerance OR if we are already at the minimum step size
                if (max_voltage_change <= tolerance || h == min_h) {
                    step_accepted = true;
                    solver.updateCircuitState(X, mnaCircuit);
                    current_time += h;

                    // Update the "previous" values for all reactive components for the next step
                    for (auto& node : mnaCircuit.getAllNodesInCircuit()) {
                        node->updateVoltageForNextStep();
                    }
                    for (auto& ind : mnaCircuit.getOrderedInductors()) {
                        ind->updateCurrentForNextStep();
                    }

                    // --- Print results for this accepted step ---
                    cout << current_time << "\t\t";
                    for (const auto& node : nonGroundNodes) {
                        cout << node->getVoltage() << "\t\t";
                    }
                    cout << "(h=" << h << ")" << endl;

                    // Dynamically increase h for the next step if the change was very small
                    if (max_voltage_change < tolerance / 10.0 && h < max_h) {
                        h *= h_increase_factor;
                        if (h > max_h) h = max_h;
                    }
                } else {
                    // Reject the step, reduce h, and retry
                    h /= h_decrease_factor;
                }

                // Safety break to prevent infinite loops if h becomes pathologically small
                if (h < 1e-18) {
                    cerr << "Error: Timestep has become excessively small. Aborting analysis to prevent infinite loop." << endl;
                    return;
                }
            }
        }
    }
};


// Example main function to demonstrate the TransientAnalysis class
int main() {
    try {
        // --- Setup for the RC Circuit from the PDF ---
        // V1 -- R1 -- (node 1) -- C1 -- GND
        Node n_source_plus("source+");
        Node n_1("1");
        Node n_gnd_rc("0_rc");

        // Set initial conditions for the node (capacitor voltage is initially 0)
        n_1.setVoltage(0.0);
        n_1.setPreviousVoltage(0.0);

        // Create elements
        VoltageSource V1(&n_source_plus, &n_gnd_rc, "V1", 5.0);
        Resistor R1_rc(&n_source_plus, &n_1, "R1", 1000.0);
        Capacitor C1_rc(&n_1, &n_gnd_rc, "C1", 1e-6);

        // Build the circuit using the MNA helper class
        MakingMNA mna_rc_circuit;
        mna_rc_circuit.addNode(&n_source_plus);
        mna_rc_circuit.addNode(&n_1);
        mna_rc_circuit.addNode(&n_gnd_rc);
        mna_rc_circuit.setGroundNode(&n_gnd_rc);
        mna_rc_circuit.addElement(&V1);
        mna_rc_circuit.addElement(&R1_rc);
        mna_rc_circuit.addElement(&C1_rc);

        cout << "--- Running Adaptive Time Step Simulation for RC Circuit ---" << endl;
        // Create the analysis driver with adaptive parameters:
        // TransientAnalysis(circuit, initial_step, min_step, max_step, voltage_tolerance)
        TransientAnalysis adaptive_sim(mna_rc_circuit, 1e-6, 1e-9, 1e-4, 0.01);

        // Run the simulation until t=0.005s (5ms)
        adaptive_sim.run(0.005);

    } catch (const std::exception& e) {
        cerr << "\n*** An exception occurred: " << e.what() << " ***" << endl;
        return 1;
    }
    return 0;
}