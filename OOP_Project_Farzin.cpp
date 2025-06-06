#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <map>
#include <cmath>
#include "Eigen/Dense" // Core Eigen library for dense matrices and vectors

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

class Node {
private:
    double voltage;
    string name;
    double previousVoltage;

public:
    Node(const string &name, double voltage = 0.0, double prev_voltage = 0.0)
            : voltage(voltage), name(name), previousVoltage(prev_voltage) {}

    string getName() const { return name; }
    double getVoltage() const { return voltage; }
    void setVoltage(double v) { voltage = v; }
    double getPreviousVoltage() const { return previousVoltage; }
    void setPreviousVoltage(double pv) { previousVoltage = pv; }
    void updateVoltageForNextStep() { previousVoltage = voltage; }
    bool isGround() const { return name == "0" || name == "GND" || name == "gnd"; }
};

class Element {
protected:
    Node *node1, *node2;
    string name;
public:
    Element(Node* n1, Node* n2, const string &name) : node1(n1), node2(n2), name(name) {
        if (!n1 || !n2) throw std::invalid_argument("Element nodes cannot be null for element: " + name);
    }
    virtual ~Element() = default;
    string getName() const { return name; }
    Node* getNode1() const { return node1; }
    Node* getNode2() const { return node2; }
    virtual string getType() const = 0;
    virtual double getValue() const { return 0.0; }
    virtual double getCurrent() const { return 0.0; }
    virtual void setCurrent(double current) { (void)current; }
    virtual void updateValue(double time) { (void)time; }
};

class Resistor : public Element {
private: double resistance;
public:
    Resistor(Node* n1, Node* n2, const string &name, double res) : Element(n1, n2, name) {
        if (res <= 0) throw std::invalid_argument("Resistance must be positive for " + name);
        this->resistance = res;
    }
    string getType() const override { return "Resistor"; }
    double getValue() const override { return resistance; }
    void setResistance(double res) { if (res > 0) this->resistance = res; }
    double getCurrent() const override { return (node1->getVoltage() - node2->getVoltage()) / resistance; }
};

class VoltageSource : public Element {
public:
    enum SourceType { DC, SIN, PULSE };
private:
    double value;
    double currentThroughSource;
    SourceType sourceType;
    double dcOffset, amplitude, frequency;
    double initialValue, pulsedValue, delayTime, riseTime, fallTime, onTime, period;
public:
    VoltageSource(Node* n1, Node* n2, const string &name, double dc_val)
            : Element(n1, n2, name), value(dc_val), currentThroughSource(0.0), sourceType(DC) {}

    void setSinParams(double offset, double amp, double freq) {
        sourceType = SIN;
        dcOffset = offset;
        amplitude = amp;
        frequency = freq > 0 ? freq : 0;
    }

    void setPulseParams(double v1, double v2, double td, double tr, double tf, double ton, double tper) {
        sourceType = PULSE;
        initialValue = v1;
        pulsedValue = v2;
        delayTime = td >= 0 ? td : 0;
        riseTime = tr > 0 ? tr : 1e-9;
        fallTime = tf > 0 ? tf : 1e-9;
        onTime = ton >= 0 ? ton : 0;
        if (tper <= 0) throw std::invalid_argument("Pulse period for " + name + " must be positive.");
        period = tper;
    }

    void updateValue(double time) override {
        if (sourceType == SIN) {
            value = dcOffset + amplitude * sin(2 * M_PI * frequency * time);
        } else if (sourceType == PULSE) {
            if (time < delayTime) {
                value = initialValue;
                return;
            }
            double timeInCycle = fmod(time - delayTime, period);
            if (timeInCycle <= riseTime) {
                value = initialValue + (pulsedValue - initialValue) * (timeInCycle / riseTime);
            } else if (timeInCycle <= riseTime + onTime) {
                value = pulsedValue;
            } else if (timeInCycle <= riseTime + onTime + fallTime) {
                value = pulsedValue - (pulsedValue - initialValue) * ((timeInCycle - riseTime - onTime) / fallTime);
            } else {
                value = initialValue;
            }
        }
    }

    void setVoltageValue(double val) {
        sourceType = DC;
        value = val;
    }
    string getType() const override { return "VoltageSource"; }
    double getValue() const override { return value; }
    double getCurrent() const override { return currentThroughSource; }
    void setCurrent(double current) override { this->currentThroughSource = current; }
};

class CurrentSource : public Element {
private: double currentValue;
public:
    CurrentSource(Node* n1, Node* n2, const string &name, double val) : Element(n1, n2, name), currentValue(val) {}
    string getType() const override { return "CurrentSource"; }
    double getValue() const override { return currentValue; }
};

class Capacitor : public Element {
private: double capacitance;
public:
    Capacitor(Node* n1, Node* n2, const string &name, double cap) : Element(n1, n2, name) {
        if (cap <= 0) throw std::invalid_argument("Capacitance must be positive for " + name);
        this->capacitance = cap;
    }
    string getType() const override { return "Capacitor"; }
    double getValue() const override { return capacitance; }
};

class Inductor : public Element {
private: double inductance, current, previousCurrent;
public:
    Inductor(Node* n1, Node* n2, const string &name, double ind)
            : Element(n1, n2, name), inductance(ind), current(0.0), previousCurrent(0.0) {
        if (ind <= 0) throw std::invalid_argument("Inductance must be positive for " + name);
    }
    string getType() const override { return "Inductor"; }
    double getValue() const override { return inductance; }
    double getCurrent() const override { return current; }
    void setCurrent(double c) override { current = c; }
    double getPreviousCurrent() const { return previousCurrent; }
    void updateCurrentForNextStep() { previousCurrent = current; }
};

class IdealDiode : public Element {
public: enum State { ON, OFF };
private: double forwardVoltage; State currentState; double current;
public:
    IdealDiode(Node* n1, Node* n2, const string& name, double vf)
            : Element(n1, n2, name), forwardVoltage(vf), currentState(OFF), current(0.0) {
        if (vf < 0) throw std::invalid_argument("Diode forward voltage must be non-negative for " + name);
    }
    string getType() const override { return "IdealDiode"; }
    double getForwardVoltage() const { return forwardVoltage; }
    State getState() const { return currentState; }
    void setState(State state) { currentState = state; }
    double getCurrent() const override { return current; }
    void setCurrent(double c) override { current = c; }
};

class VCVS : public Element {
private: Node* controlNode1; Node* controlNode2; double gain; double current;
public:
    VCVS(Node* n1, Node* n2, const string& name, Node* cn1, Node* cn2, double g)
            : Element(n1, n2, name), controlNode1(cn1), controlNode2(cn2), gain(g), current(0.0) {}
    string getType() const override { return "VCVS"; }
    double getValue() const override { return gain; }
    Node* getControlNode1() const { return controlNode1; }
    Node* getControlNode2() const { return controlNode2; }
    double getCurrent() const override { return current; }
    void setCurrent(double c) override { current = c; }
};

class VCCS : public Element {
private: Node* controlNode1; Node* controlNode2; double gain;
public:
    VCCS(Node* n1, Node* n2, const string& name, Node* cn1, Node* cn2, double g)
            : Element(n1, n2, name), controlNode1(cn1), controlNode2(cn2), gain(g) {}
    string getType() const override { return "VCCS"; }
    double getValue() const override { return gain; }
    Node* getControlNode1() const { return controlNode1; }
    Node* getControlNode2() const { return controlNode2; }
};

class CCVS : public Element {
private: string controlVoltageSourceName; double gain; double current;
public:
    CCVS(Node* n1, Node* n2, const string& name, const string& cvs_name, double g)
            : Element(n1, n2, name), controlVoltageSourceName(cvs_name), gain(g), current(0.0) {}
    string getType() const override { return "CCVS"; }
    double getValue() const override { return gain; }
    string getControlVoltageSourceName() const { return controlVoltageSourceName; }
    double getCurrent() const override { return current; }
    void setCurrent(double c) override { current = c; }
};

class CCCS : public Element {
private: string controlVoltageSourceName; double gain;
public:
    CCCS(Node* n1, Node* n2, const string& name, const string& cvs_name, double g)
            : Element(n1, n2, name), controlVoltageSourceName(cvs_name), gain(g) {}
    string getType() const override { return "CCCS"; }
    double getValue() const override { return gain; }
    string getControlVoltageSourceName() const { return controlVoltageSourceName; }
};

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
    map<VCVS *, int> vcvsToIndexMap;
    vector<VCVS *> orderedVCVS;
    map<CCVS *, int> ccvsToIndexMap;
    vector<CCVS *> orderedCCVS;
    double timeStep_h;

    void buildSystemMaps() {
        nodeToIndexMap.clear(); orderedNonGroundNodes.clear();
        vsToIndexMap.clear(); orderedVoltageSources.clear();
        inductorToIndexMap.clear(); orderedInductors.clear();
        idealDiodeToIndexMap.clear(); orderedIdealDiodes.clear();
        vcvsToIndexMap.clear(); orderedVCVS.clear();
        ccvsToIndexMap.clear(); orderedCCVS.clear();

        if (!groundNodeRef) {
            for (Node *n: allNodesInCircuit) if (n->isGround()) { groundNodeRef = n; break; }
            if (!groundNodeRef) throw std::runtime_error("Error: Ground node not detected.");
        }

        int nodeIdx = 0;
        for (Node *node: allNodesInCircuit) {
            if (!node->isGround()) {
                orderedNonGroundNodes.push_back(node);
                nodeToIndexMap[node] = nodeIdx++;
            }
        }

        for (Element *elem: elementsInCircuit) {
            if (auto vs = dynamic_cast<VoltageSource *>(elem)) orderedVoltageSources.push_back(vs);
            else if (auto ind = dynamic_cast<Inductor *>(elem)) orderedInductors.push_back(ind);
            else if (auto id = dynamic_cast<IdealDiode *>(elem)) orderedIdealDiodes.push_back(id);
            else if (auto vcvs = dynamic_cast<VCVS *>(elem)) orderedVCVS.push_back(vcvs);
            else if (auto ccvs = dynamic_cast<CCVS *>(elem)) orderedCCVS.push_back(ccvs);
        }

        for (size_t i = 0; i < orderedVoltageSources.size(); ++i) vsToIndexMap[orderedVoltageSources[i]] = i;
        for (size_t i = 0; i < orderedInductors.size(); ++i) inductorToIndexMap[orderedInductors[i]] = i;
        for (size_t i = 0; i < orderedIdealDiodes.size(); ++i) idealDiodeToIndexMap[orderedIdealDiodes[i]] = i;
        for (size_t i = 0; i < orderedVCVS.size(); ++i) vcvsToIndexMap[orderedVCVS[i]] = i;
        for (size_t i = 0; i < orderedCCVS.size(); ++i) ccvsToIndexMap[orderedCCVS[i]] = i;
    }

public:
    MakingMNA(double h = -1.0) : groundNodeRef(nullptr), timeStep_h(h) {}
    ~MakingMNA() {}

    void setTimeStep(double h) { this->timeStep_h = h; }
    double getTimeStep() const { return this->timeStep_h; }
    void addNode(Node* node) { if (node) allNodesInCircuit.push_back(node); }
    void addElement(Element* element) { if(element) elementsInCircuit.push_back(element); }
    void setGroundNode(Node* gnd) { groundNodeRef = gnd; if(gnd) addNode(gnd); }
    Element* getElement(const string& name) {
        for (auto* elem : elementsInCircuit) if (elem->getName() == name) return elem;
        return nullptr;
    }
    const vector<Node*>& getOrderedNonGroundNodes() const { return orderedNonGroundNodes; }
    const vector<Element*>& getAllElements() const { return elementsInCircuit; }
    const vector<Node*>& getAllNodesInCircuit() const { return allNodesInCircuit; }
    const vector<VoltageSource*>& getOrderedVoltageSources() const { return orderedVoltageSources; }
    const vector<Inductor*>& getOrderedInductors() const { return orderedInductors; }
    const vector<IdealDiode*>& getOrderedIdealDiodes() const { return orderedIdealDiodes; }
    const vector<VCVS*>& getOrderedVCVS() const { return orderedVCVS; }
    const vector<CCVS*>& getOrderedCCVS() const { return orderedCCVS; }

    Eigen::MatrixXd getSystemMatrixA(bool isDCAnalysis = false) {
        buildSystemMaps();

        int numNonGroundNodes = orderedNonGroundNodes.size();
        int numVS = orderedVoltageSources.size();
        int numL = orderedInductors.size();
        int numD = orderedIdealDiodes.size();
        int numVCVS = orderedVCVS.size();
        int numCCVS = orderedCCVS.size();
        int systemSize = numNonGroundNodes + numVS + numL + numD + numVCVS + numCCVS;

        if (systemSize == 0) return Eigen::MatrixXd(0,0);
        Eigen::MatrixXd A = Eigen::MatrixXd::Zero(systemSize, systemSize);

        for (Element* elem : elementsInCircuit) {
            Node* n1 = elem->getNode1();
            Node* n2 = elem->getNode2();

            if (auto res = dynamic_cast<Resistor*>(elem)) {
                double g = 1.0 / res->getValue();
                if (!n1->isGround()) A(nodeToIndexMap[n1], nodeToIndexMap[n1]) += g;
                if (!n2->isGround()) A(nodeToIndexMap[n2], nodeToIndexMap[n2]) += g;
                if (!n1->isGround() && !n2->isGround()) {
                    A(nodeToIndexMap[n1], nodeToIndexMap[n2]) -= g;
                    A(nodeToIndexMap[n2], nodeToIndexMap[n1]) -= g;
                }
            }
            else if (auto cap = dynamic_cast<Capacitor*>(elem)) {
                if (!isDCAnalysis) {
                    if (this->timeStep_h <= 0) throw std::runtime_error("Time step h is not set for capacitor " + cap->getName());
                    double g = cap->getValue() / this->timeStep_h;
                    if (!n1->isGround()) A(nodeToIndexMap[n1], nodeToIndexMap[n1]) += g;
                    if (!n2->isGround()) A(nodeToIndexMap[n2], nodeToIndexMap[n2]) += g;
                    if (!n1->isGround() && !n2->isGround()) {
                        A(nodeToIndexMap[n1], nodeToIndexMap[n2]) -= g;
                        A(nodeToIndexMap[n2], nodeToIndexMap[n1]) -= g;
                    }
                }
            }
            else if (auto vs = dynamic_cast<VoltageSource*>(elem)) {
                int i = numNonGroundNodes + vsToIndexMap[vs];
                if (!n1->isGround()) { A(nodeToIndexMap[n1], i) += 1.0; A(i, nodeToIndexMap[n1]) += 1.0; }
                if (!n2->isGround()) { A(nodeToIndexMap[n2], i) -= 1.0; A(i, nodeToIndexMap[n2]) -= 1.0; }
            }
            else if (auto ind = dynamic_cast<Inductor*>(elem)) {
                int i = numNonGroundNodes + numVS + inductorToIndexMap[ind];
                if (isDCAnalysis) {
                    if (!n1->isGround()) A(i, nodeToIndexMap[n1]) += 1.0;
                    if (!n2->isGround()) A(i, nodeToIndexMap[n2]) -= 1.0;
                } else {
                    double l_div_h = ind->getValue() / this->timeStep_h;
                    if (!n1->isGround()) { A(nodeToIndexMap[n1], i) += 1.0; A(i, nodeToIndexMap[n1]) += 1.0; }
                    if (!n2->isGround()) { A(nodeToIndexMap[n2], i) -= 1.0; A(i, nodeToIndexMap[n2]) -= 1.0; }
                    A(i, i) -= l_div_h;
                }
            }
            else if (auto diode = dynamic_cast<IdealDiode*>(elem)) {
                int i = numNonGroundNodes + numVS + numL + idealDiodeToIndexMap[diode];
                if (diode->getState() == IdealDiode::ON) {
                    if (!n1->isGround()) { A(nodeToIndexMap[n1], i) += 1.0; A(i, nodeToIndexMap[n1]) += 1.0; }
                    if (!n2->isGround()) { A(nodeToIndexMap[n2], i) -= 1.0; A(i, nodeToIndexMap[n2]) -= 1.0; }
                } else { A(i, i) = 1.0; }
            }
            else if (auto vccs = dynamic_cast<VCCS*>(elem)) {
                double g = vccs->getValue();
                Node* cn1 = vccs->getControlNode1(); Node* cn2 = vccs->getControlNode2();
                if (!n1->isGround() && !cn1->isGround()) A(nodeToIndexMap[n1], nodeToIndexMap[cn1]) += g;
                if (!n1->isGround() && !cn2->isGround()) A(nodeToIndexMap[n1], nodeToIndexMap[cn2]) -= g;
                if (!n2->isGround() && !cn1->isGround()) A(nodeToIndexMap[n2], nodeToIndexMap[cn1]) -= g;
                if (!n2->isGround() && !cn2->isGround()) A(nodeToIndexMap[n2], nodeToIndexMap[cn2]) += g;
            }
            else if (auto cccs = dynamic_cast<CCCS*>(elem)) {
                double gain = cccs->getValue();
                VoltageSource* ctrl_vs = nullptr;
                for(auto* vs_ptr : orderedVoltageSources) if(vs_ptr->getName() == cccs->getControlVoltageSourceName()) ctrl_vs = vs_ptr;
                if (!ctrl_vs) throw std::runtime_error("Error: Dependent source '" + cccs->getName() + "' has an undefined control element '" + cccs->getControlVoltageSourceName() + "'.");
                int ctrl_i = numNonGroundNodes + vsToIndexMap[ctrl_vs];
                if (!n1->isGround()) A(nodeToIndexMap[n1], ctrl_i) += gain;
                if (!n2->isGround()) A(nodeToIndexMap[n2], ctrl_i) -= gain;
            }
            else if (auto vcvs = dynamic_cast<VCVS*>(elem)) {
                int i = numNonGroundNodes + numVS + numL + numD + vcvsToIndexMap[vcvs];
                Node* cn1 = vcvs->getControlNode1(); Node* cn2 = vcvs->getControlNode2();
                double gain = vcvs->getValue();
                if (!n1->isGround()) { A(nodeToIndexMap[n1], i) += 1.0; A(i, nodeToIndexMap[n1]) += 1.0; }
                if (!n2->isGround()) { A(nodeToIndexMap[n2], i) -= 1.0; A(i, nodeToIndexMap[n2]) -= 1.0; }
                if (!cn1->isGround()) A(i, nodeToIndexMap[cn1]) -= gain;
                if (!cn2->isGround()) A(i, nodeToIndexMap[cn2]) += gain;
            }
            else if (auto ccvs = dynamic_cast<CCVS*>(elem)) {
                int i = numNonGroundNodes + numVS + numL + numD + numVCVS + ccvsToIndexMap[ccvs];
                double gain = ccvs->getValue();
                VoltageSource* ctrl_vs = nullptr;
                for(auto* vs_ptr : orderedVoltageSources) if(vs_ptr->getName() == ccvs->getControlVoltageSourceName()) ctrl_vs = vs_ptr;
                if (!ctrl_vs) throw std::runtime_error("Error: Dependent source '" + ccvs->getName() + "' has an undefined control element '" + ccvs->getControlVoltageSourceName() + "'.");
                int ctrl_i = numNonGroundNodes + vsToIndexMap[ctrl_vs];
                if (!n1->isGround()) A(i, nodeToIndexMap[n1]) += 1.0;
                if (!n2->isGround()) A(i, nodeToIndexMap[n2]) -= 1.0;
                A(i, ctrl_i) -= gain;
            }
        }
        return A;
    }

    Eigen::VectorXd getSystemVectorZ(bool isDCAnalysis = false) {
        int systemSize = orderedNonGroundNodes.size() + orderedVoltageSources.size() + orderedInductors.size() + orderedIdealDiodes.size() + orderedVCVS.size() + orderedCCVS.size();
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

        for (const auto& vs : orderedVoltageSources) Z(orderedNonGroundNodes.size() + vsToIndexMap[vs]) = vs->getValue();
        for (const auto& ind : orderedInductors) {
            int i = orderedNonGroundNodes.size() + orderedVoltageSources.size() + inductorToIndexMap[ind];
            if (!isDCAnalysis) {
                Z(i) = - (ind->getValue() / this->timeStep_h) * ind->getPreviousCurrent();
            }
        }
        for (const auto& diode : orderedIdealDiodes) {
            int i = orderedNonGroundNodes.size() + orderedVoltageSources.size() + orderedInductors.size() + idealDiodeToIndexMap[diode];
            if (diode->getState() == IdealDiode::ON) Z(i) = diode->getForwardVoltage();
        }
        return Z;
    }
};

class MNASolver {
public:
    MNASolver() {}
    Eigen::VectorXd solve(const Eigen::MatrixXd& A, const Eigen::VectorXd& Z) {
        if (A.rows() == 0 && Z.size() == 0) return Eigen::VectorXd(0);
        if (A.rows() != A.cols() || A.rows() != Z.size()) throw std::runtime_error("Matrix/vector dimensions incompatible.");
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
        const auto& vcvs_sources = mnaCircuit.getOrderedVCVS();
        const auto& ccvs_sources = mnaCircuit.getOrderedCCVS();

        int numNonGroundNodes = nonGroundNodes.size();
        int numVS = voltageSources.size();
        int numL = inductors.size();
        int numD = idealDiodes.size();
        int numVCVS = vcvs_sources.size();

        if (static_cast<size_t>(X.size()) != numNonGroundNodes+numVS+numL+numD+numVCVS+ccvs_sources.size()) {
            throw std::runtime_error("Solution vector size does not match number of unknowns.");
        }

        for (size_t i = 0; i < nonGroundNodes.size(); ++i) nonGroundNodes[i]->setVoltage(X(i));
        for (size_t i = 0; i < voltageSources.size(); ++i) voltageSources[i]->setCurrent(X(numNonGroundNodes + i));
        for (size_t i = 0; i < inductors.size(); ++i) inductors[i]->setCurrent(X(numNonGroundNodes + numVS + i));
        for (size_t i = 0; i < idealDiodes.size(); ++i) idealDiodes[i]->setCurrent(X(numNonGroundNodes + numVS + numL + i));
        for (size_t i = 0; i < vcvs_sources.size(); ++i) vcvs_sources[i]->setCurrent(X(numNonGroundNodes + numVS + numL + numD + i));
        for (size_t i = 0; i < ccvs_sources.size(); ++i) ccvs_sources[i]->setCurrent(X(numNonGroundNodes + numVS + numL + numD + numVCVS + i));
    }
};

class TransientAnalysis {
private:
    MakingMNA& mnaCircuit;
    MNASolver solver;
    double initial_h, min_h, max_h, tolerance, h_increase_factor, h_decrease_factor;
public:
    TransientAnalysis(MakingMNA& circuit, double initial_step, double min_step, double max_step, double tol)
            : mnaCircuit(circuit), solver(), initial_h(initial_step), min_h(min_step),
              max_h(max_step), tolerance(tol), h_increase_factor(1.5), h_decrease_factor(2.0) {
        if (min_h <= 0 || max_h < min_h || initial_h < min_h || initial_h > max_h) {
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
                elem->updateValue(current_time);
            }

            bool diodes_converged = false;
            int max_diode_iterations = 25;
            int iteration_count = 0;
            while (!diodes_converged && iteration_count < max_diode_iterations) {
                iteration_count++;
                diodes_converged = true;

                mnaCircuit.setTimeStep(h);
                Eigen::MatrixXd A = mnaCircuit.getSystemMatrixA();
                Eigen::VectorXd Z = mnaCircuit.getSystemVectorZ();
                Eigen::VectorXd X = solver.solve(A, Z);
                solver.updateCircuitState(X, mnaCircuit);

                for (auto& diode_ptr : mnaCircuit.getOrderedIdealDiodes()) {
                    IdealDiode::State old_state = diode_ptr->getState();
                    double v_anode = diode_ptr->getNode1()->getVoltage();
                    double v_cathode = diode_ptr->getNode2()->getVoltage();
                    if (old_state == IdealDiode::OFF) {
                        if (v_anode > v_cathode + diode_ptr->getForwardVoltage()) {
                            diode_ptr->setState(IdealDiode::ON);
                            diodes_converged = false;
                        }
                    } else {
                        if (diode_ptr->getCurrent() < 0) {
                            diode_ptr->setState(IdealDiode::OFF);
                            diodes_converged = false;
                        }
                    }
                }
            }
            if (!diodes_converged) cout << "Warning: Diode states failed to converge at t=" << current_time << endl;

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
                    if (h < min_h) h = min_h;
                }
                if (h < 1e-18) {
                    cerr << "Error: Timestep excessively small. Aborting." << endl;
                    return;
                }
            }
        }
    }
};

class DCSweepAnalysis {
private:
    MakingMNA& mnaCircuit;
    MNASolver solver;
    string sweepComponentName;
    double startValue, endValue, increment;
public:
    DCSweepAnalysis(MakingMNA& circuit, const string& compName, double start, double end, double inc)
            : mnaCircuit(circuit), solver(), sweepComponentName(compName), startValue(start), endValue(end), increment(inc) {
        if (increment <= 0) throw std::invalid_argument("DC sweep increment must be positive.");
    }
    void run() {
        Element* sweepElement = mnaCircuit.getElement(sweepComponentName);
        if (!sweepElement) throw std::runtime_error("Sweep component '" + sweepComponentName + "' not found.");
        auto* sweepVoltageSource = dynamic_cast<VoltageSource*>(sweepElement);
        auto* sweepResistor = dynamic_cast<Resistor*>(sweepElement);
        if (!sweepVoltageSource && !sweepResistor) throw std::runtime_error("Sweep component must be a VoltageSource or a Resistor.");

        cout << sweepComponentName << "\t";
        const auto& nonGroundNodes = mnaCircuit.getOrderedNonGroundNodes();
        for (const auto& node : nonGroundNodes) cout << "V(" << node->getName() << ")\t";
        cout << endl;
        cout << "-----------------------------------------------------" << endl;

        for (double val = startValue; val <= endValue; val += increment) {
            if (sweepVoltageSource) sweepVoltageSource->setVoltageValue(val);
            else if (sweepResistor) sweepResistor->setResistance(val);

            Eigen::MatrixXd A = mnaCircuit.getSystemMatrixA(true);
            Eigen::VectorXd Z = mnaCircuit.getSystemVectorZ(true);
            Eigen::VectorXd X = solver.solve(A, Z);
            solver.updateCircuitState(X, mnaCircuit);

            cout << val << "\t";
            for (const auto& node : nonGroundNodes) cout << node->getVoltage() << "\t";
            cout << endl;
        }
    }
};