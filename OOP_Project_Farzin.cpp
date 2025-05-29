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

public:
    Node(const string &name, double voltage = 0.0) {
        this->name = name;
        this->voltage = voltage;
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

    bool isGround() const {
        return name == "0" || name == "GND" || name == "gnd";
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
    MakingMNA() : groundNodeRef(nullptr) {}

    ~MakingMNA() {
        // Assuming memory management of nodes and elements is handled outside this class
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

        if (systemSize == 0) {
            throw std::runtime_error("Error: Circuit is too small for analysis (no non-ground nodes or voltage sources).");
        }

        Eigen::MatrixXd A = Eigen::MatrixXd::Zero(systemSize, systemSize);

        // G part (conductances from resistors)
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
        }

        // B and C parts (voltage sources)
        for (size_t i = 0; i < orderedVoltageSources.size(); ++i) {
            VoltageSource* vs = orderedVoltageSources[i];
            Node* n_plus = vs->getNode1();
            Node* n_minus = vs->getNode2();
            int vsMNAIndex = vsToIndexMap[vs]; // Index for this voltage source's current unknown

            if (n_plus != groundNodeRef) {
                int nodeIdx = nodeToIndexMap[n_plus];
                A(nodeIdx, numNonGroundNodes + vsMNAIndex) += 1.0;  // B part
                A(numNonGroundNodes + vsMNAIndex, nodeIdx) += 1.0;  // C part
            }
            if (n_minus != groundNodeRef) {
                int nodeIdx = nodeToIndexMap[n_minus];
                A(nodeIdx, numNonGroundNodes + vsMNAIndex) -= 1.0; // B part
                A(numNonGroundNodes + vsMNAIndex, nodeIdx) -= 1.0; // C part
            }
        }
        // D part is zero for ideal independent voltage sources, already initialized by Zero()

        return A;
    }

    Eigen::VectorXd getSystemVectorZ() {
        // Assumes buildNodeAndVoltageSourceMaps() has been called
        int numNonGroundNodes = orderedNonGroundNodes.size();
        int numVoltageSources = orderedVoltageSources.size();
        int systemSize = numNonGroundNodes + numVoltageSources;

        if (systemSize == 0 && numNonGroundNodes == 0) {
            Eigen::VectorXd Z_empty(0);
            return Z_empty;
        }

        Eigen::VectorXd Z = Eigen::VectorXd::Zero(systemSize);

        // J part (current sources)
        for (Element* elem : elementsInCircuit) {
            if (auto cs = dynamic_cast<CurrentSource*>(elem)) {
                Node* n_from = cs->getNode1(); // Current leaves this node
                Node* n_to = cs->getNode2();   // Current enters this node
                double currentValue = cs->getValue();

                if (n_to != groundNodeRef) {
                    Z(nodeToIndexMap[n_to]) += currentValue;
                }
                if (n_from != groundNodeRef) {
                    Z(nodeToIndexMap[n_from]) -= currentValue;
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
        if (A.rows() != A.cols() || A.rows() != Z.size()) {
            throw std::runtime_error("Error: Matrix and vector dimensions are not compatible for solving.");
        }
        if (A.rows() == 0) {
            throw std::runtime_error("Error: System of equations is empty.");
        }

        // Using LU decomposition with partial pivoting for numerical stability
        Eigen::PartialPivLU<Eigen::MatrixXd> lu(A);
        // Check for singularity using the determinant.
        if (std::abs(lu.determinant()) < 1e-9) { // MODIFIED LINE
            throw std::runtime_error("Error: System matrix is singular or ill-conditioned (determinant is near zero). The circuit may not be solvable (e.g., floating sections, redundant voltage sources).");
        }
        // test for invertibility in this context.

        return lu.solve(Z);
    }

    void updateCircuitState(const Eigen::VectorXd& X, MakingMNA& mnaCircuit) {
        const auto& nonGroundNodes = mnaCircuit.getOrderedNonGroundNodes();
        const auto& voltageSources = mnaCircuit.getOrderedVoltageSources();
        const auto& vsMap = mnaCircuit.getVoltageSourceToIndexMap();

        int numNonGroundNodes = nonGroundNodes.size();

        if (X.size() != numNonGroundNodes + voltageSources.size()) {
            throw std::runtime_error("Error: Solution vector size does not match the number of unknowns.");
        }

        // Update node voltages
        for (int i = 0; i < numNonGroundNodes; ++i) {
            nonGroundNodes[i]->setVoltage(X(i));
        }

        // Update currents through voltage sources
        for (size_t i = 0; i < voltageSources.size(); ++i) {
            VoltageSource* vs = voltageSources[i];
            // Ensure vs is in vsMap before accessing.
            // buildNodeAndVoltageSourceMaps in getSystemMatrixA should ensure this.
            int vsMNAIndex = vsMap.at(vs); // This is the 0-based index relative to the start of VS unknowns
            vs->setCurrent(X(numNonGroundNodes + vsMNAIndex));
        }
    }
};

// Main function for testing
int main() {
    // Set output precision for floating-point numbers
    cout << fixed << setprecision(6);

    // 1. Create nodes
    Node n1("1"), n2("2"), n_gnd("0"); // "0" or "GND" is typically ground

    // 2. Create circuit manager and add nodes
    MakingMNA circuit;
    circuit.addNode(&n1);
    circuit.addNode(&n2);
    circuit.addNode(&n_gnd);
    // circuit.setGroundNode(&n_gnd); // Explicitly set ground (though it's also auto-detected)

    // 3. Create elements and add them to the circuit
    try {
        // Example from PDF section 9 (RC circuit): V1=5V, R=1k, C=1uF (here C is replaced with a second resistor for a DC example)
        // V1 between node 1 and ground, R1 between 1 and 2, R2 between 2 and ground
        VoltageSource vs(&n1, &n_gnd, "V1", 5.0);   // 5V voltage source between node 1 and ground
        Resistor r1(&n1, &n2, "R1", 1000.0);     // 1 kOhm resistor between node 1 and 2
        Resistor r2(&n2, &n_gnd, "R2", 2000.0);   // 2 kOhm resistor between node 2 and ground
        // CurrentSource cs(&n_gnd, &n2, "I1", 0.001); // Example: 1mA current source from ground to node 2

        circuit.addElement(&vs);
        circuit.addElement(&r1);
        circuit.addElement(&r2);
        // circuit.addElement(&cs);

        // 4. Get MNA matrices
        Eigen::MatrixXd A = circuit.getSystemMatrixA();
        Eigen::VectorXd Z = circuit.getSystemVectorZ();

        cout << "System Matrix A:\n" << A << endl << endl;
        cout << "System Vector Z:\n" << Z << endl << endl;

        // 5. Solve the system
        MNASolver solver;
        Eigen::VectorXd X = solver.solve(A, Z);

        cout << "Solution Vector X (node voltages then voltage source currents):\n" << X << endl << endl;

        // 6. Update node voltages and voltage source currents in their respective objects
        solver.updateCircuitState(X, circuit);

        // 7. Print results
        cout << "Node Voltages after solving:" << endl;
        for (const auto* node : circuit.getOrderedNonGroundNodes()) {
            cout << "Node " << node->getName() << ": " << node->getVoltage() << " V" << endl;
        }

        cout << "\nCurrents through Voltage Sources:" << endl;
        for (const auto* vs_elem : circuit.getOrderedVoltageSources()) {
            cout << "Current through " << vs_elem->getName() << ": " << vs_elem->getCurrent() << " A" << endl;
        }

        cout << "\nCurrents through Resistors (calculated after solving):" << endl;
        for (const auto* elem : circuit.getAllElements()) {
            if (const Resistor* res = dynamic_cast<const Resistor*>(elem)) {
                cout << "Current through " << res->getName() << " (" << res->getNode1()->getName() << "->" << res->getNode2()->getName() << "): "
                     << res->getCurrent() << " A" << endl;
            }
        }

    } catch (const std::exception& e) {
        cerr << "An error occurred: " << e.what() << endl;
    }

    return 0;
}