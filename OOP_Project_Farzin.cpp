#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <Eigen/Dense>

// Enum to represent the type of circuit element
enum class ElementType {
    RESISTOR,
    CAPACITOR,
    INDUCTOR,
    VOLTAGE_SOURCE_INDEPENDENT,
    CURRENT_SOURCE_INDEPENDENT,
    VOLTAGE_SOURCE_DEPENDENT_VCVS,
    CURRENT_SOURCE_DEPENDENT_CCCS,
    VOLTAGE_SOURCE_DEPENDENT_CCVS,
    CURRENT_SOURCE_DEPENDENT_VCCS,
    DIODE, //
    GROUND //
    // Add other types as needed
};

// Structure to store information for each circuit element
class CircuitElement {
public:
    ElementType type;
    std::string name; // Unique identifier for the element
    int node1;        // First connected node
    int node2;        // Second connected node
    double value;     // Value of the element (e.g., resistance, voltage)

    // For dependent sources
    int controlNode1; // For VCVS, VCCS
    int controlNode2; // For VCVS, VCCS
    std::string controllingElementName; // For CCVS, CCCS (name of the V-source whose current is controlling)
    double gain;       // Gain for dependent sources

    // Default constructor
    CircuitElement() : type(ElementType::RESISTOR), node1(0), node2(0), value(0.0),
                       controlNode1(0), controlNode2(0), gain(0.0) {}

    // Constructor for basic elements
    CircuitElement(ElementType t, std::string n, int n1, int n2, double val)
            : type(t), name(std::move(n)), node1(n1), node2(n2), value(val),
              controlNode1(0), controlNode2(0), gain(0.0) {
        // Assuming node 0 is ground, as suggested
    }

    // Constructor for VCVS/VCCS (Voltage Controlled)
    CircuitElement(ElementType t, std::string n, int n1, int n2, int cn1, int cn2, double g)
            : type(t), name(std::move(n)), node1(n1), node2(n2), value(0.0), // value might not be used directly
              controlNode1(cn1), controlNode2(cn2), controllingElementName(""), gain(g) {
        if (t != ElementType::VOLTAGE_SOURCE_DEPENDENT_VCVS && t != ElementType::CURRENT_SOURCE_DEPENDENT_VCCS) {
            // Throw error or handle mismatch
        }
    }

    // Constructor for CCVS/CCCS (Current Controlled)
    CircuitElement(ElementType t, std::string n, int n1, int n2, std::string ctrlName, double g)
            : type(t), name(std::move(n)), node1(n1), node2(n2), value(0.0), // value might not be used directly
              controlNode1(0), controlNode2(0), controllingElementName(std::move(ctrlName)), gain(g) {
        if (t != ElementType::VOLTAGE_SOURCE_DEPENDENT_CCVS && t != ElementType::CURRENT_SOURCE_DEPENDENT_CCCS) {
            // Throw error or handle mismatch
        }
    }


    void print() const {
        std::cout << "Element: " << name << ", Type: " << static_cast<int>(type)
                  << ", Node1: " << node1 << ", Node2: " << node2
                  << ", Value: " << value;
        if (type == ElementType::VOLTAGE_SOURCE_DEPENDENT_VCVS || type == ElementType::CURRENT_SOURCE_DEPENDENT_VCCS) {
            std::cout << ", CtrlNode1: " << controlNode1 << ", CtrlNode2: " << controlNode2 << ", Gain: " << gain;
        } else if (type == ElementType::VOLTAGE_SOURCE_DEPENDENT_CCVS || type == ElementType::CURRENT_SOURCE_DEPENDENT_CCCS) {
            std::cout << ", ControllingElement: " << controllingElementName << ", Gain: " << gain;
        }
        std::cout << std::endl;
    }
};

// Class to represent the entire circuit
class Circuit {
public:
    std::vector<CircuitElement> elements;
    std::set<int> nodes; // Stores unique node numbers, 0 is typically ground
    int groundNode = 0; // Explicitly define ground node

    void addElement(const CircuitElement& elem) {
        elements.push_back(elem);
        if (elem.node1 != groundNode) nodes.insert(elem.node1); // Add nodes, excluding ground if managed separately
        if (elem.node2 != groundNode) nodes.insert(elem.node2);
        // For dependent sources, control nodes also need to be considered if they are part of the circuit's nodes
        if (elem.type == ElementType::VOLTAGE_SOURCE_DEPENDENT_VCVS || elem.type == ElementType::CURRENT_SOURCE_DEPENDENT_VCCS) {
            if (elem.controlNode1 != groundNode) nodes.insert(elem.controlNode1);
            if (elem.controlNode2 != groundNode) nodes.insert(elem.controlNode2);
        }
    }

    // Example function to get the number of non-ground nodes
    int getNumNonGroundNodes() const {
        return nodes.size();
    }

    // Example function to get number of independent voltage sources
    int getNumIndependentVoltageSources() const {
        int count = 0;
        for (const auto& elem : elements) {
            if (elem.type == ElementType::VOLTAGE_SOURCE_INDEPENDENT) {
                count++;
            }
        }
        return count;
    }


    void printCircuit() const {
        std::cout << "Circuit Elements:" << std::endl;
        for (const auto& elem : elements) {
            elem.print();
        }
        std::cout << "Circuit Nodes (excluding ground " << groundNode << "): ";
        for (int node : nodes) {
            std::cout << node << " ";
        }
        std::cout << std::endl;
    }
};

// --- MNASystem using Eigen ---
class MNASystem {
public:
    Eigen::MatrixXd A; // MNA Matrix using Eigen
    Eigen::VectorXd b; // RHS Vector using Eigen

    int numNonGroundNodes;
    int numIndVoltageSources;
    const Circuit* circuit_ptr;

    std::map<int, int> nodeToIndexMap; // Maps circuit node ID to 0-indexed matrix row/col
    std::map<std::string, int> vSourceNameToCurrentIndexMap; // Maps V-source name to its current variable index

    MNASystem(const Circuit& c) : circuit_ptr(&c) {
        numNonGroundNodes = 0;
        int currentIndex = 0;
        for(int node_num : c.nodes) {
            if (node_num != c.groundNode) {
                nodeToIndexMap[node_num] = currentIndex++;
            }
        }
        numNonGroundNodes = currentIndex;

        numIndVoltageSources = 0;
        for (const auto& elem : c.elements) {
            if (elem.elementType && dynamic_cast<VoltageSourceIndependentElementType*>(elem.elementType.get())) {
                // The index for current variables starts after all node voltage variables
                vSourceNameToCurrentIndexMap[elem.name] = numNonGroundNodes + numIndVoltageSources;
                numIndVoltageSources++;
            }
        }

        int matrixSize = numNonGroundNodes + numIndVoltageSources;
        if (matrixSize > 0) {
            A = Eigen::MatrixXd::Zero(matrixSize, matrixSize);
            b = Eigen::VectorXd::Zero(matrixSize);
        } else {
            // Handle empty circuit or circuit with only ground
            std::cerr << "Warning: MNA system size is 0." << std::endl;
        }
    }

    // Get matrix index for a node number
    int getIndex(int node) {
        if (node == circuit_ptr->groundNode) return -1; // Ground node doesn't have a direct row/col
        auto it = nodeToIndexMap.find(node);
        if (it != nodeToIndexMap.end()) {
            return it->second;
        }
        std::cerr << "Error: Node " << node << " not found in map." << std::endl;
        return -2; // Error indicator
    }

    void buildMatrices() {
        if (!circuit_ptr || A.size() == 0) { // Ensure circuit_ptr is valid and matrix is initialized
            if (A.size() == 0 && (numNonGroundNodes + numIndVoltageSources > 0)) {
                std::cerr << "Error: MNA Matrix not properly initialized." << std::endl;
            }
            return;
        }

        for (const auto& elem : circuit_ptr->elements) {
            if (!elem.elementType) continue;

            int n1_idx = getIndex(elem.node1);
            int n2_idx = getIndex(elem.node2);

            if (dynamic_cast<ResistorElementType*>(elem.elementType.get())) {
                if (elem.value == 0) {
                    std::cerr << "Error: Resistor " << elem.name << " has zero resistance." << std::endl;
                    continue; // Avoid division by zero
                }
                double conductance = 1.0 / elem.value;
                if (n1_idx != -1) { // If node1 is not ground
                    A(n1_idx, n1_idx) += conductance;
                    if (n2_idx != -1) { // If node2 is also not ground
                        A(n1_idx, n2_idx) -= conductance;
                        A(n2_idx, n1_idx) -= conductance;
                        A(n2_idx, n2_idx) += conductance;
                    }
                } else { // node1 is ground
                    if (n2_idx != -1) { // node2 is not ground
                        A(n2_idx, n2_idx) += conductance;
                    }
                }
            } else if (dynamic_cast<CurrentSourceIndependentElementType*>(elem.elementType.get())) {
                if (n1_idx != -1) { // Current entering n1 from source
                    b(n1_idx) -= elem.value;
                }
                if (n2_idx != -1) { // Current leaving n2 into source
                    b(n2_idx) += elem.value;
                }
            } else if (dynamic_cast<VoltageSourceIndependentElementType*>(elem.elementType.get())) {
                auto it = vSourceNameToCurrentIndexMap.find(elem.name);
                if (it == vSourceNameToCurrentIndexMap.end()) {
                    std::cerr << "Error: Voltage source " << elem.name << " not mapped to a current index." << std::endl;
                    continue;
                }
                int currentVarIdx = it->second; // This is the index for the V-source's current variable

                // KCL contributions (B matrix part)
                if (n1_idx != -1) { // Positive terminal
                    A(n1_idx, currentVarIdx) += 1.0;
                }
                if (n2_idx != -1) { // Negative terminal
                    A(n2_idx, currentVarIdx) -= 1.0;
                }

                // Branch equation for the voltage source (C matrix part and E vector part)
                // V_n1 - V_n2 = Value  =>  1*V_n1 - 1*V_n2 = Value
                // This is A(currentVarIdx, voltage_indices_involved) = coefficients
                // and b(currentVarIdx) = source_value
                if (n1_idx != -1) {
                    A(currentVarIdx, n1_idx) += 1.0;
                }
                if (n2_idx != -1) {
                    A(currentVarIdx, n2_idx) -= 1.0;
                }
                b(currentVarIdx) = elem.value; // E vector part
            }
            // TODO: Add stamps for other elements (Capacitors, Inductors for transient, Dependent Sources)
        }
    }

    // --- Solvers using Eigen ---

    // Solve Ax = b using Eigen's PartialPivLU decomposition (similar to Gaussian Elimination)
    Eigen::VectorXd solveWithPartialPivLU() {
        if (A.rows() == 0 || A.cols() == 0) {
            std::cerr << "Error: Matrix A is empty or not initialized for LU solver." << std::endl;
            return Eigen::VectorXd();
        }
        if (A.rows() != b.size()) {
            std::cerr << "Error: Matrix A and vector b dimensions mismatch for LU solver." << std::endl;
            return Eigen::VectorXd();
        }
        // Check if the matrix is square
        if (A.rows() != A.cols()) {
            std::cerr << "Error: Matrix A is not square, cannot use PartialPivLU directly. Consider QR decomposition for non-square systems." << std::endl;
            return Eigen::VectorXd();
        }

        Eigen::PartialPivLU<Eigen::MatrixXd> lu(A);
        if (lu.info() != Eigen::Success) {
            std::cerr << "Error: LU decomposition failed. Matrix might be singular." << std::endl;
            return Eigen::VectorXd();
        }
        Eigen::VectorXd x = lu.solve(b);
        if (lu.info() != Eigen::Success) { // Check solve status
            std::cerr << "Error: Solving Ax=b after LU decomposition failed." << std::endl;
            return Eigen::VectorXd();
        }
        return x;
    }

    // Solve Ax = b using Eigen's QR decomposition (robust, can handle non-square matrices too)
    Eigen::VectorXd solveWithQR() {
        if (A.rows() == 0 || A.cols() == 0) {
            std::cerr << "Error: Matrix A is empty or not initialized for QR solver." << std::endl;
            return Eigen::VectorXd();
        }
        if (A.rows() != b.size()) {
            std::cerr << "Error: Matrix A and vector b dimensions mismatch for QR solver." << std::endl;
            return Eigen::VectorXd();
        }
        // ColPivHouseholderQR is robust for rank-deficient matrices as well
        Eigen::ColPivHouseholderQR<Eigen::MatrixXd> qr(A);
        if (qr.info() != Eigen::Success) {
            std::cerr << "Error: QR decomposition failed." << std::endl;
            return Eigen::VectorXd();
        }
        Eigen::VectorXd x = qr.solve(b);
        if (qr.info() != Eigen::Success) { // Check solve status
            std::cerr << "Error: Solving Ax=b after QR decomposition failed." << std::endl;
            return Eigen::VectorXd();
        }
        return x;
    }
};