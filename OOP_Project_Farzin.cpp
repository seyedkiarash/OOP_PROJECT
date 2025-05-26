#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <algorithm>

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