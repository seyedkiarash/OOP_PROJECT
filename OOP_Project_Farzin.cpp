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
               name == "0_pdf9" || name == "0_pdf10";
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
    virtual double getValue() const { return 0.0; }
    virtual double getCurrent() const {
        return 0.0;
    }
    virtual void setCurrent(double current) {
        (void)current;
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

    // New method to allow modification for parameter sweeps
    void setResistance(double res) {
        if (res <= 0) {
            throw std::invalid_argument("Error: Resistance value must be positive for " + name);
        }
        this->resistance = res;
    }

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
        this->currentThroughSource = 0.0;
    }

    string getType() const override { return "VoltageSource"; }
    double getValue() const override { return voltageValue; }

    void setVoltageValue(double val) {
        this->voltageValue = val;
    }

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
    double current;
    double previousCurrent;

public:
    Inductor(Node* n1, Node* n2, const string &name, double ind)
            : Element(n1, n2, name), inductance(ind), current(0.0), previousCurrent(0.0) {
        if (ind <= 0) {
            throw std::invalid_argument("Error: Inductance value must be positive. Inductor: " + name);
        }
    }

    string getType() const override { return "Inductor"; }
    double getValue() const override { return inductance; }

    double getCurrent() const override { return current; }
    void setCurrent(double c) { current = c; }

    double getPreviousCurrent() const { return previousCurrent; }
    void setPreviousCurrent(double pc) { previousCurrent = pc; }

    void updateCurrentForNextStep() {
        previousCurrent = current;
    }
};

// Ideal Diode Class
class IdealDiode : public Element {
public:
    enum State { ON, OFF };

private:
    double forwardVoltage;
    State currentState;
    double current;

public:
    IdealDiode(Node* n1, Node* n2, const string& name, double vf)
            : Element(n1, n2, name), forwardVoltage(vf), currentState(OFF), current(0.0) {
        if (vf < 0) {
            throw std::invalid_argument("Diode forward voltage cannot be negative for diode: " + name);
        }
    }

    string getType() const override { return "IdealDiode"; }
    double getForwardVoltage() const { return forwardVoltage; }
    State getState() const { return currentState; }
    void setState(State state) { currentState = state; }

    double getCurrent() const override { return current; }
    void setCurrent(double c) override { current = c; }
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
    map<IdealDiode *, int> idealDiodeToIndexMap;
    vector<IdealDiode *> orderedIdealDiodes;

    double timeStep_h;

    void buildSystemMaps() {
        nodeToIndexMap.clear();
        orderedNonGroundNodes.clear();
        vsToIndexMap.clear();
        orderedVoltageSources.clear();
        inductorToIndexMap.clear();
        orderedInductors.clear();
        idealDiodeToIndexMap.clear();
        orderedIdealDiodes.clear();

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

        for (Element *elem: elementsInCircuit) {
            if (auto vs = dynamic_cast<VoltageSource *>(elem)) {
                orderedVoltageSources.push_back(vs);
            } else if (auto ind = dynamic_cast<Inductor *>(elem)) {
                orderedInductors.push_back(ind);
            } else if (auto id = dynamic_cast<IdealDiode *>(elem)) {
                orderedIdealDiodes.push_back(id);
            }
        }

        for (size_t i = 0; i < orderedVoltageSources.size(); ++i) vsToIndexMap[orderedVoltageSources[i]] = i;
        for (size_t i = 0; i < orderedInductors.size(); ++i) inductorToIndexMap[orderedInductors[i]] = i;
        for (size_t i = 0; i < orderedIdealDiodes.size(); ++i) idealDiodeToIndexMap[orderedIdealDiodes[i]] = i;
    }

public:
    MakingMNA(double h = -1.0) : groundNodeRef(nullptr), timeStep_h(h) {}

    ~MakingMNA() {
    }

    void setTimeStep(double h) {
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

    Element* getElement(const string& name) {
        for (auto* elem : elementsInCircuit) {
            if (elem->getName() == name) {
                return elem;
            }
        }
        return nullptr;
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

    Eigen::MatrixXd getSystemMatrixA(bool isDCAnalysis = false) {
        buildSystemMaps();

        int numNonGroundNodes = orderedNonGroundNodes.size();
        int numVoltageSources = orderedVoltageSources.size();
        int numInductors = orderedInductors.size();
        int numIdealDiodes = orderedIdealDiodes.size();
        int systemSize = numNonGroundNodes + numVoltageSources + numInductors + numIdealDiodes;

        if (systemSize == 0) return Eigen::MatrixXd(0,0);

        Eigen::MatrixXd A = Eigen::MatrixXd::Zero(systemSize, systemSize);

        for (Element* elem : elementsInCircuit) {
            Node* n1 = elem->getNode1();
            Node* n2 = elem->getNode2();
            double conductance = 0;

            if (auto res = dynamic_cast<Resistor*>(elem)) {
                conductance = 1.0 / res->getValue();
            } else if (auto cap = dynamic_cast<Capacitor*>(elem)) {
                if (!isDCAnalysis) { // Capacitors are open circuits in DC
                    if (this->timeStep_h <= 0) throw std::runtime_error("Time step h is not set for capacitor " + cap->getName());
                    conductance = cap->getValue() / this->timeStep_h;
                }
            }

            if (conductance != 0) {
                if (!n1->isGround()) A(nodeToIndexMap[n1], nodeToIndexMap[n1]) += conductance;
                if (!n2->isGround()) A(nodeToIndexMap[n2], nodeToIndexMap[n2]) += conductance;
                if (!n1->isGround() && !n2->isGround()) {
                    A(nodeToIndexMap[n1], nodeToIndexMap[n2]) -= conductance;
                    A(nodeToIndexMap[n2], nodeToIndexMap[n1]) -= conductance;
                }
            }
        }

        for (const auto& vs : orderedVoltageSources) {
            int vsCurrentVarIndex = numNonGroundNodes + vsToIndexMap[vs];
            int vsBranchEqRow = vsCurrentVarIndex;
            if (!vs->getNode1()->isGround()) {
                A(nodeToIndexMap[vs->getNode1()], vsCurrentVarIndex) += 1.0;
                A(vsBranchEqRow, nodeToIndexMap[vs->getNode1()]) += 1.0;
            }
            if (!vs->getNode2()->isGround()) {
                A(nodeToIndexMap[vs->getNode2()], vsCurrentVarIndex) -= 1.0;
                A(vsBranchEqRow, nodeToIndexMap[vs->getNode2()]) -= 1.0;
            }
        }

        for (const auto& ind : orderedInductors) {
            int base_idx = numNonGroundNodes + numVoltageSources;
            int indCurrentVarIndex = base_idx + inductorToIndexMap[ind];
            int indBranchEqRow = indCurrentVarIndex;
            if (isDCAnalysis) { // Inductors are short circuits in DC (0V drop)
                if (!ind->getNode1()->isGround()) A(indBranchEqRow, nodeToIndexMap[ind->getNode1()]) += 1.0;
                if (!ind->getNode2()->isGround()) A(indBranchEqRow, nodeToIndexMap[ind->getNode2()]) -= 1.0;
            } else { // Transient analysis
                if (this->timeStep_h <= 0) throw std::runtime_error("Time step h is not set for inductor " + ind->getName());
                double l_div_h = ind->getValue() / this->timeStep_h;
                if (!ind->getNode1()->isGround()) {
                    A(nodeToIndexMap[ind->getNode1()], indCurrentVarIndex) += 1.0;
                    A(indBranchEqRow, nodeToIndexMap[ind->getNode1()]) += 1.0;
                }
                if (!ind->getNode2()->isGround()) {
                    A(nodeToIndexMap[ind->getNode2()], indCurrentVarIndex) -= 1.0;
                    A(indBranchEqRow, nodeToIndexMap[ind->getNode2()]) -= 1.0;
                }
                A(indBranchEqRow, indCurrentVarIndex) -= l_div_h;
            }
        }

        for (const auto& diode : orderedIdealDiodes) {
            int base_idx = numNonGroundNodes + numVoltageSources + numInductors;
            int diodeCurrentVarIndex = base_idx + idealDiodeToIndexMap[diode];
            int diodeBranchEqRow = diodeCurrentVarIndex;

            if (diode->getState() == IdealDiode::ON) {
                if (!diode->getNode1()->isGround()) {
                    A(nodeToIndexMap[diode->getNode1()], diodeCurrentVarIndex) += 1.0;
                    A(diodeBranchEqRow, nodeToIndexMap[diode->getNode1()]) += 1.0;
                }
                if (!diode->getNode2()->isGround()) {
                    A(nodeToIndexMap[diode->getNode2()], diodeCurrentVarIndex) -= 1.0;
                    A(diodeBranchEqRow, nodeToIndexMap[diode->getNode2()]) -= 1.0;
                }
            } else {
                A(diodeBranchEqRow, diodeCurrentVarIndex) = 1.0;
            }
        }

        return A;
    }

    Eigen::VectorXd getSystemVectorZ(bool isDCAnalysis = false) {
        int numNonGroundNodes = orderedNonGroundNodes.size();
        int numVoltageSources = orderedVoltageSources.size();
        int numInductors = orderedInductors.size();
        int numIdealDiodes = orderedIdealDiodes.size();
        int systemSize = numNonGroundNodes + numVoltageSources + numInductors + numIdealDiodes;

        if (systemSize == 0) return Eigen::VectorXd(0);

        Eigen::VectorXd Z = Eigen::VectorXd::Zero(systemSize);

        for (Element* elem : elementsInCircuit) {
            if (auto cs = dynamic_cast<CurrentSource*>(elem)) {
                if (!cs->getNode1()->isGround()) Z(nodeToIndexMap[cs->getNode1()]) -= cs->getValue();
                if (!cs->getNode2()->isGround()) Z(nodeToIndexMap[cs->getNode2()]) += cs->getValue();
            } else if (auto cap = dynamic_cast<Capacitor*>(elem)) {
                if (!isDCAnalysis) {
                    if (this->timeStep_h <= 0) throw std::runtime_error("Time step h is not set for capacitor " + cap->getName());
                    double c_div_h = cap->getValue() / this->timeStep_h;
                    double v_n1_prev = cap->getNode1()->getPreviousVoltage();
                    double v_n2_prev = cap->getNode2()->getPreviousVoltage();
                    double i_eq_cap = c_div_h * (v_n1_prev - v_n2_prev);
                    if (!cap->getNode1()->isGround()) Z(nodeToIndexMap[cap->getNode1()]) += i_eq_cap;
                    if (!cap->getNode2()->isGround()) Z(nodeToIndexMap[cap->getNode2()]) -= i_eq_cap;
                }
            }
        }

        for (const auto& vs : orderedVoltageSources) {
            Z(numNonGroundNodes + vsToIndexMap[vs]) = vs->getValue();
        }

        for (const auto& ind : orderedInductors) {
            int indBranchEqRow = numNonGroundNodes + numVoltageSources + inductorToIndexMap[ind];
            if (isDCAnalysis) {
                Z(indBranchEqRow) = 0; // V_L = 0 in DC
            } else {
                if (this->timeStep_h <= 0) throw std::runtime_error("Time step h is not set for inductor " + ind->getName());
                double l_div_h = ind->getValue() / this->timeStep_h;
                Z(indBranchEqRow) = -(l_div_h * ind->getPreviousCurrent());
            }
        }

        for (const auto& diode : orderedIdealDiodes) {
            int base_idx = numNonGroundNodes + numVoltageSources + numInductors;
            int diodeBranchEqRow = base_idx + idealDiodeToIndexMap[diode];
            if (diode->getState() == IdealDiode::ON) {
                Z(diodeBranchEqRow) = diode->getForwardVoltage();
            } else {
                Z(diodeBranchEqRow) = 0.0;
            }
        }

        return Z;
    }

    const vector<Node*>& getOrderedNonGroundNodes() const { return orderedNonGroundNodes; }
    const vector<Node*>& getAllNodesInCircuit() const { return allNodesInCircuit; }
    const vector<VoltageSource*>& getOrderedVoltageSources() const { return orderedVoltageSources; }
    const vector<Inductor*>& getOrderedInductors() const { return orderedInductors; }
    const vector<IdealDiode*>& getOrderedIdealDiodes() const { return orderedIdealDiodes; }

    const vector<Element*>& getAllElements() const { return elementsInCircuit; }
};

// MNASolver class
class MNASolver {
public:
    MNASolver() {}

    Eigen::VectorXd solve(const Eigen::MatrixXd& A, const Eigen::VectorXd& Z) {
        if (A.rows() == 0 && Z.size() == 0) return Eigen::VectorXd(0);
        if (A.rows() != A.cols() || A.rows() != Z.size()) {
            throw std::runtime_error("Matrix/vector dimensions incompatible for solving.");
        }
        if (A.rows() == 0) throw std::runtime_error("System of equations is empty.");

        Eigen::PartialPivLU<Eigen::MatrixXd> lu(A);
        if (A.rows() > 0 && std::abs(lu.determinant()) < 1e-14) {
            cout << "Warning: System matrix determinant is near zero. Matrix may be singular." << endl;
        }
        return lu.solve(Z);
    }

    void updateCircuitState(const Eigen::VectorXd& X, MakingMNA& mnaCircuit) {
        const auto& nonGroundNodes = mnaCircuit.getOrderedNonGroundNodes();
        const auto& voltageSources = mnaCircuit.getOrderedVoltageSources();
        const auto& inductors = mnaCircuit.getOrderedInductors();
        const auto& idealDiodes = mnaCircuit.getOrderedIdealDiodes();

        int numNonGroundNodes = nonGroundNodes.size();
        int numVoltageSources = voltageSources.size();
        int numInductors = inductors.size();

        if (static_cast<size_t>(X.size()) != numNonGroundNodes + numVoltageSources + numInductors + idealDiodes.size()) {
            throw std::runtime_error("Solution vector size does not match number of unknowns.");
        }

        for (size_t i = 0; i < nonGroundNodes.size(); ++i) nonGroundNodes[i]->setVoltage(X(i));
        for (size_t i = 0; i < voltageSources.size(); ++i) voltageSources[i]->setCurrent(X(numNonGroundNodes + i));
        for (size_t i = 0; i < inductors.size(); ++i) inductors[i]->setCurrent(X(numNonGroundNodes + numVoltageSources + i));
        for (size_t i = 0; i < idealDiodes.size(); ++i) idealDiodes[i]->setCurrent(X(numNonGroundNodes + numVoltageSources + numInductors + i));
    }
};

// ===================================================================================
// ===== NEW AND MODIFIED ANALYSIS CLASSES ===========================================
// ===================================================================================


// TransientAnalysis class
class TransientAnalysis {
private:
    MakingMNA& mnaCircuit;
    MNASolver solver;

    double initial_h, min_h, max_h, tolerance, h_increase_factor, h_decrease_factor;

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

        cout << "Time (s)\t";
        const auto& nonGroundNodes = mnaCircuit.getOrderedNonGroundNodes();
        for (const auto& node : nonGroundNodes) cout << "V(" << node->getName() << ")\t";
        for (const auto& vs : mnaCircuit.getOrderedVoltageSources()) cout << "I(" << vs->getName() << ")\t";
        cout << "(h)" << endl;
        cout << "----------------------------------------------------------------" << endl;


        while (current_time < t_stop) {
            if (current_time + h > t_stop) h = t_stop - current_time;

            for (auto& elem : mnaCircuit.getAllElements()) {
                if (auto vs = dynamic_cast<VoltageSource*>(elem)) {
                    if (vs->getName() == "Vsin") {
                        vs->setVoltageValue(5.0 * sin(2 * M_PI * 60 * current_time));
                    }
                }
            }


            bool diodes_converged = false;
            int max_diode_iterations = 25;
            int iteration_count = 0;

            while (!diodes_converged && iteration_count < max_diode_iterations) {
                iteration_count++;
                diodes_converged = true;

                Eigen::MatrixXd A = mnaCircuit.getSystemMatrixA();
                Eigen::VectorXd Z = mnaCircuit.getSystemVectorZ();
                Eigen::VectorXd X = solver.solve(A, Z);
                solver.updateCircuitState(X, mnaCircuit);

                for (auto& diode : mnaCircuit.getOrderedIdealDiodes()) {
                    IdealDiode::State old_state = diode->getState();
                    double v_anode = diode->getNode1()->getVoltage();
                    double v_cathode = diode->getNode2()->getVoltage();

                    if (old_state == IdealDiode::OFF) {
                        if (v_anode > v_cathode + diode->getForwardVoltage()) {
                            diode->setState(IdealDiode::ON);
                            diodes_converged = false;
                        }
                    } else {
                        if (diode->getCurrent() < 0) {
                            diode->setState(IdealDiode::OFF);
                            diodes_converged = false;
                        }
                    }
                }
            }
            if (iteration_count >= max_diode_iterations) {
                cout << "Warning: Diode states failed to converge at t=" << current_time << endl;
            }


            std::vector<double> old_voltages;
            for(const auto& node : nonGroundNodes) old_voltages.push_back(node->getVoltage());

            bool step_accepted = false;
            while (!step_accepted) {
                if (h < min_h) h = min_h;
                mnaCircuit.setTimeStep(h);
                Eigen::MatrixXd A = mnaCircuit.getSystemMatrixA();
                Eigen::VectorXd Z = mnaCircuit.getSystemVectorZ();
                Eigen::VectorXd X = solver.solve(A, Z);

                double max_voltage_change = 0.0;
                for (size_t i = 0; i < nonGroundNodes.size(); ++i) {
                    double change = std::abs(X(i) - old_voltages[i]);
                    if (change > max_voltage_change) max_voltage_change = change;
                }

                if (max_voltage_change <= tolerance || h == min_h) {
                    step_accepted = true;
                    solver.updateCircuitState(X, mnaCircuit);
                    current_time += h;

                    for (auto& node : mnaCircuit.getAllNodesInCircuit()) node->updateVoltageForNextStep();
                    for (auto& ind : mnaCircuit.getOrderedInductors()) ind->updateCurrentForNextStep();

                    cout << current_time << "\t";
                    for (const auto& node : nonGroundNodes) cout << node->getVoltage() << "\t";
                    for (const auto& vs : mnaCircuit.getOrderedVoltageSources()) cout << vs->getCurrent() << "\t";
                    cout << "(h=" << h << ")" << endl;

                    if (max_voltage_change < tolerance / 10.0 && h < max_h) {
                        h *= h_increase_factor;
                        if (h > max_h) h = max_h;
                    }
                } else {
                    h /= h_decrease_factor;
                }
                if (h < 1e-18) {
                    cerr << "Error: Timestep excessively small. Aborting." << endl;
                    return;
                }
            }
        }
    }
};

// New Class for DC Sweep Analysis
class DCSweepAnalysis {
private:
    MakingMNA& mnaCircuit;
    MNASolver solver;
    string sweepComponentName;
    double startValue, endValue, increment;

public:
    DCSweepAnalysis(MakingMNA& circuit, const string& compName, double start, double end, double inc)
            : mnaCircuit(circuit), solver(), sweepComponentName(compName), startValue(start), endValue(end), increment(inc) {
        if (increment <= 0) {
            throw std::invalid_argument("DC sweep increment must be positive.");
        }
    }

    void run() {
        Element* sweepElement = mnaCircuit.getElement(sweepComponentName);
        if (!sweepElement) {
            throw std::runtime_error("Sweep component '" + sweepComponentName + "' not found.");
        }

        auto* sweepVoltageSource = dynamic_cast<VoltageSource*>(sweepElement);
        auto* sweepResistor = dynamic_cast<Resistor*>(sweepElement);

        if (!sweepVoltageSource && !sweepResistor) {
            throw std::runtime_error("Sweep component must be a VoltageSource or a Resistor.");
        }

        cout << sweepComponentName << "\t";
        for (const auto& node : mnaCircuit.getOrderedNonGroundNodes()) {
            cout << "V(" << node->getName() << ")\t";
        }
        cout << endl;
        cout << "-----------------------------------------------------" << endl;

        for (double val = startValue; val <= endValue; val += increment) {
            if (sweepVoltageSource) {
                sweepVoltageSource->setVoltageValue(val);
            } else if (sweepResistor) {
                sweepResistor->setResistance(val);
            }

            // A DC analysis does not require diode state iteration
            // unless the DC solution itself is non-linear, which we assume is not the case here
            // for simplicity. A full implementation might need the diode loop here as well.

            Eigen::MatrixXd A = mnaCircuit.getSystemMatrixA(true); // true for DC analysis
            Eigen::VectorXd Z = mnaCircuit.getSystemVectorZ(true); // true for DC analysis
            Eigen::VectorXd X = solver.solve(A, Z);
            solver.updateCircuitState(X, mnaCircuit);

            cout << val << "\t";
            for (const auto& node : mnaCircuit.getOrderedNonGroundNodes()) {
                cout << node->getVoltage() << "\t";
            }
            cout << endl;
        }
    }
};


// Main function
int main() {
    try {
        cout << "===== DC Sweep Analysis of a Voltage Divider =====" << endl;

        // --- Setup for a voltage divider: V1 -- R1 -- (out) -- R2 -- GND ---
        Node n_in("in_dc");
        Node n_out("out_dc");
        Node n_gnd_dc("0_dc");

        VoltageSource V1(&n_in, &n_gnd_dc, "V1", 1.0); // Initial value doesn't matter, will be swept
        Resistor R1(&n_in, &n_out, "R1", 1000.0);
        Resistor R2(&n_out, &n_gnd_dc, "R2", 1000.0);

        MakingMNA mna_divider_circuit;
        mna_divider_circuit.addNode(&n_in);
        mna_divider_circuit.addNode(&n_out);
        mna_divider_circuit.addNode(&n_gnd_dc);
        mna_divider_circuit.setGroundNode(&n_gnd_dc);
        mna_divider_circuit.addElement(&V1);
        mna_divider_circuit.addElement(&R1);
        mna_divider_circuit.addElement(&R2);

        // Sweep V1 from 0V to 10V in 1V increments
        DCSweepAnalysis dc_sweep(mna_divider_circuit, "V1", 0.0, 10.0, 1.0);
        dc_sweep.run();

        cout << "\n\n===== Transient Analysis of Half-Wave Rectifier =====" << endl;
        Node n_tr_in("in");
        Node n_tr_out("out");
        Node n_tr_gnd("0");

        n_tr_out.setVoltage(0.0);
        n_tr_out.setPreviousVoltage(0.0);

        VoltageSource Vsin(&n_tr_in, &n_tr_gnd, "Vsin", 0.0);
        IdealDiode D1(&n_tr_in, &n_tr_out, "D1", 0.7);
        Resistor R_load(&n_tr_out, &n_tr_gnd, "R_load", 1000.0);

        MakingMNA mna_rectifier_circuit;
        mna_rectifier_circuit.addNode(&n_tr_in);
        mna_rectifier_circuit.addNode(&n_tr_out);
        mna_rectifier_circuit.addNode(&n_tr_gnd);
        mna_rectifier_circuit.setGroundNode(&n_tr_gnd);
        mna_rectifier_circuit.addElement(&Vsin);
        mna_rectifier_circuit.addElement(&D1);
        mna_rectifier_circuit.addElement(&R_load);

        TransientAnalysis adaptive_sim(mna_rectifier_circuit, 1e-5, 1e-7, 1e-4, 0.05);
        adaptive_sim.run(2.0 / 60.0);


    } catch (const std::exception& e) {
        cerr << "\n*** An exception occurred: " << e.what() << " ***" << endl;
        return 1;
    }
    return 0;
}