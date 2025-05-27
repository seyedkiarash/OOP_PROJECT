// #include <bits/stdc++.h>
//
// using namespace std;
//
// class Node {
// private:
//     int voltage;
//     string name;
// public:
//     Node(const string &name,int voltage) {
//         this->name = name;
//         this->voltage = voltage;
//     }
//     string getName() {
//         return name;
//     }
//     double getVoltage() {
//         return voltage;
//     }
//     void setVoltage(double v) {
//         voltage = v;
//     }
// };
//
// class Element {
// protected:
//     Node *node1,*node2;
//     string name;
//     public:
//     Element(Node* n1,Node* n2,const string &name) {
//         this->node1 = n1;
//         this->node2 = n2;
//         this->name = name;
//     }
//     virtual ~Element() = default;
//     virtual double getVoltage() = 0;
//     virtual string getName() = 0;
//     virtual double getCurrent() = 0;
//     virtual string getType() = 0;
//     virtual void setCurrent(double current) = 0;
//     virtual double getValue() = 0;
//     Node* getNode1() {
//         return node1;
//     }
//     Node* getNode2() {
//         return node2;
//     }
// };
//
// class makingMNA {
// private:
//     vector<Node*> nodes;
//     vector<Element*> elements;
//     Node* ground;
//     public:
//     makingMNA() {
//         nodes={};
//         elements={};
//         ground=nullptr;
//     }
//     ~makingMNA() {
//         nodes.clear();
//         elements.clear();
//         ground=nullptr;
//     }
//     vector<Node*> findingActiveNodes() {
//         vector<Node*> activeNode;
//         for (auto &n:nodes) {
//             if (n!=ground)
//                 activeNode.push_back(n);
//         }
//         return activeNode;
//     }
//     vector<vector<Element*>> getElementConnectedToNodes() {
//         vector<vector<Element*>> elementConnectToNodes;
//         for (auto &node:nodes) {
//             vector<Element*> temp;
//             for (auto&e:elements) {
//                 if (node==e->getNode1()||node==e->getNode2())
//                     temp.push_back(e);
//             }
//             elementConnectToNodes.push_back(temp);
//         }
//         return elementConnectToNodes;
//     }
//     vector<Element*> getElementBeetWeenNodes(int i,int j) {
//         vector<Element*> elementBeetWeenNodes;
//         for (auto &e:elements) {
//             if (e->getNode1()==nodes[i]&&e->getNode2()==nodes[j])
//                 elementBeetWeenNodes.push_back(e);
//         }
//         return elementBeetWeenNodes;
//     }
//     vector<vector<double>> createGMatrix() {
//         vector<Node*> activeNode=findingActiveNodes();
//         vector<vector<Element*>> elementConnectToNodes=getElementConnectedToNodes();
//         int n=activeNode.size();
//         vector<vector<double>> G(n,vector<double>(n,0.0));
//         for (int i=0;i<n;i++) {
//             for (int j=0;j<n;j++) {
//                 if (i==j) {
//                     for (int k=0;k<elementConnectToNodes[i].size();k++) {
//                         if (elementConnectToNodes[i][k]->getType()=="Resistor")
//                             G[i][j]+=1/elementConnectToNodes[i][k]->getValue();
//                     }
//                 }
//             }
//         }
//         for (int i=0;i<n;i++) {
//             for (int j=0;j<n;j++) {
//                 if (i!=j) {
//                     vector<Element*> temp=getElementBeetWeenNodes(i,j);
//                     for (int k=0;k<elementConnectToNodes.size();k++) {
//                         if (temp[k]->getType()=="Resistor")
//                             G[i][j]+=-1.0/temp[k]->getValue();
//                     }
//                 }
//             }
//         }
//         return G;
//     }
//
// };
///////////////////////////////////////////////
// #include <iomanip>   // برای форматирование вывода (std::fixed, std::setprecision)
// #include <iostream>
// #include <map>       // برای نگاشت گره‌ها به اندیس ماتریس
// #include <stdexcept> // برای کلاس‌های استثنا مانند std::invalid_argument
// #include <string>
// #include <vector>
//
// // کتابخانه Eigen برای عملیات ماتریسی
// #include "Eigen/Dense"
//
// // استفاده از فضای نام std برای سادگی در این مثال
// // در پروژه‌های بزرگتر، بهتر است از پیشوند std:: استفاده شود یا using declarations محدودتری به کار رود.
// using namespace std;
//
// // تعریف کلاس گره (Node)
// class Node {
// private:
//     double voltage; // ولتاژ گره، به double تغییر داده شد
//     string name;    // نام گره
//
// public:
//     // سازنده کلاس گره
//     Node(const string &name, double voltage = 0.0) { // مقدار پیش‌فرض ولتاژ 0.0 در نظر گرفته شد
//         this->name = name;
//         this->voltage = voltage;
//     }
//
//     // دریافت نام گره
//     string getName() const {
//         return name;
//     }
//
//     // دریافت ولتاژ گره
//     double getVoltage() const {
//         return voltage;
//     }
//
//     // تنظیم ولتاژ گره
//     void setVoltage(double v) {
//         voltage = v;
//     }
//
//     // بررسی اینکه آیا گره، گره زمین (گراند) است یا خیر
//     bool isGround() const {
//         return name == "0" || name == "GND" || name == "gnd"; // نام‌های رایج برای زمین
//     }
// };
//
// // تعریف کلاس پایه برای المان‌های مداری (Element)
// class Element {
// protected:
//     Node *node1, *node2; // اشاره‌گر به گره‌های متصل به المان
//     string name;         // نام المان
//
// public:
//     // سازنده کلاس المان
//     Element(Node* n1, Node* n2, const string &name) {
//         if (!n1 || !n2) {
//             throw std::invalid_argument("گره‌های المان نمی‌توانند تهی (null) باشند.");
//         }
//         this->node1 = n1;
//         this->node2 = n2;
//         this->name = name;
//     }
//
//     // مخرب مجازی پیش‌فرض
//     virtual ~Element() = default;
//
//     // دریافت نام المان
//     string getName() const { return name; }
//
//     // دریافت گره اول
//     Node* getNode1() const { return node1; }
//
//     // دریافت گره دوم
//     Node* getNode2() const { return node2; }
//
//     // تابع مجازی خالص برای دریافت نوع المان (باید در کلاس‌های مشتق پیاده‌سازی شود)
//     virtual string getType() const = 0;
//
//     // تابع مجازی خالص برای دریافت مقدار مشخصه المان (مقاومت، ولتاژ منبع، جریان منبع و ...)
//     virtual double getValue() const = 0;
//
//     // تابع مجازی برای دریافت جریان عبوری از المان (می‌تواند در کلاس‌های مشتق بازنویسی شود)
//     // برای المان‌هایی مانند مقاومت پس از حل مدار محاسبه می‌شود
//     // برای منابع ولتاژ، این یکی از مجهولات سیستم MNA است
//     virtual double getCurrent() const {
//         return 0.0; // پیاده‌سازی پیش‌فرض
//     }
//
//     // تابع مجازی برای تنظیم جریان عبوری از المان (مربوط به منابع ولتاژ)
//     virtual void setCurrent(double current) {
//         // پیاده‌سازی پیش‌فرض، برای جلوگیری از هشدار پارامتر استفاده‌نشده
//         (void)current;
//     }
// };
//
// // کلاس مقاومت (Resistor) مشتق‌شده از Element
// class Resistor : public Element {
// private:
//     double resistance; // مقدار مقاومت
//
// public:
//     Resistor(Node* n1, Node* n2, const string &name, double res) : Element(n1, n2, name) {
//         if (res <= 0) {
//             // مطابق فایل PDF، بخش ۱۹ (مدیریت خطاها)
//             throw std::invalid_argument("خطا: مقدار مقاومت باید مثبت باشد. المان: " + name);
//         }
//         this->resistance = res;
//     }
//
//     string getType() const override { return "Resistor"; }
//     double getValue() const override { return resistance; } // مقدار مقاومت را برمی‌گرداند
//
//     // جریان عبوری از مقاومت طبق قانون اهم (پس از مشخص شدن ولتاژ گره‌ها)
//     double getCurrent() const override {
//         if (node1 && node2) {
//             return (node1->getVoltage() - node2->getVoltage()) / resistance;
//         }
//         return 0.0;
//     }
// };
//
// // کلاس منبع ولتاژ مستقل (VoltageSource) مشتق‌شده از Element
// class VoltageSource : public Element {
// private:
//     double voltageValue;         // مقدار ولتاژ منبع
//     double currentThroughSource; // جریان عبوری از منبع (یکی از مجهولات MNA)
//
// public:
//     VoltageSource(Node* n1, Node* n2, const string &name, double val) : Element(n1, n2, name) {
//         this->voltageValue = val;
//         this->currentThroughSource = 0.0; // مقدار اولیه
//     }
//
//     string getType() const override { return "VoltageSource"; }
//     double getValue() const override { return voltageValue; } // مقدار ولتاژ تنظیم‌شده منبع را برمی‌گرداند
//
//     double getCurrent() const override {
//         return currentThroughSource;
//     }
//     void setCurrent(double current) override {
//         this->currentThroughSource = current;
//     }
// };
//
// // کلاس منبع جریان مستقل (CurrentSource) مشتق‌شده از Element
// class CurrentSource : public Element {
// private:
//     double currentValue; // مقدار جریان منبع
//
// public:
//     // گره n1 گرهی است که جریان از آن خارج می‌شود، گره n2 گرهی است که جریان به آن وارد می‌شود
//     CurrentSource(Node* n1, Node* n2, const string &name, double val) : Element(n1, n2, name) {
//         this->currentValue = val;
//     }
//
//     string getType() const override { return "CurrentSource"; }
//     double getValue() const override { return currentValue; } // مقدار جریان تنظیم‌شده منبع را برمی‌گرداند
//
//     // ولتاژ دو سر منبع جریان توسط مدار تعیین می‌شود و جزو مجهولات مستقیم برای این المان نیست
// };
//
//
// // کلاس مسئول تشکیل ماتریس‌های MNA
// class MakingMNA {
// private:
//     vector<Node*> allNodesInCircuit;     // لیست تمام گره‌های موجود در مدار
//     vector<Element*> elementsInCircuit;  // لیست تمام المان‌های موجود در مدار
//     Node* groundNodeRef;                 // اشاره‌گر به گره زمین
//
//     // نگاشت‌ها برای تشکیل ماتریس
//     map<Node*, int> nodeToIndexMap;          // نگاشت گره‌های غیرزمین به اندیس سطر/ستون در ماتریس MNA
//     vector<Node*> orderedNonGroundNodes;     // لیست مرتب‌شده گره‌های غیرزمین
//     map<VoltageSource*, int> vsToIndexMap;   // نگاشت منابع ولتاژ به اندیس مجهول جریانشان در MNA
//     vector<VoltageSource*> orderedVoltageSources; // لیست مرتب‌شده منابع ولتاژ
//
//     // متد داخلی برای ساخت نگاشت‌ها
//     void buildNodeAndVoltageSourceMaps() {
//         nodeToIndexMap.clear();
//         orderedNonGroundNodes.clear();
//         vsToIndexMap.clear();
//         orderedVoltageSources.clear();
//
//         // تعیین گره زمین در صورت عدم تنظیم صریح
//         if (!groundNodeRef) {
//             for (Node* n : allNodesInCircuit) {
//                 if (n->isGround()) {
//                     groundNodeRef = n;
//                     break;
//                 }
//             }
//             if (!groundNodeRef) {
//                 // مطابق فایل PDF، بخش ۱۹ (مدیریت خطاها)
//                 throw std::runtime_error("خطا: گره زمین (گراند) در مدار تشخیص داده نشد. تحلیل امکان‌پذیر نیست.");
//             }
//         }
//
//         // شماره‌گذاری گره‌های غیرزمین
//         int nodeIdx = 0;
//         for (Node* node : allNodesInCircuit) {
//             if (node != groundNodeRef) {
//                 orderedNonGroundNodes.push_back(node);
//                 nodeToIndexMap[node] = nodeIdx++;
//             }
//         }
//
//         // شماره‌گذاری منابع ولتاژ (برای مجهولات جریان آنها)
//         int vsIdx = 0;
//         for (Element* elem : elementsInCircuit) {
//             if (auto vs = dynamic_cast<VoltageSource*>(elem)) {
//                 orderedVoltageSources.push_back(vs);
//                 vsToIndexMap[vs] = vsIdx++;
//             }
//         }
//     }
//
// public:
//     // سازنده
//     MakingMNA() : groundNodeRef(nullptr) {}
//
//     // مخرب (در صورت نیاز به مدیریت حافظه پیچیده‌تر، باید تکمیل شود)
//     ~MakingMNA() {
//         // اگر گره‌ها و المان‌ها در این کلاس new شده باشند، باید delete شوند.
//         // در این پیاده‌سازی، فرض بر این است که مدیریت حافظه آنها خارج از این کلاس انجام می‌شود.
//     }
//
//     // افزودن گره به مدار
//     void addNode(Node* node) {
//         if (!node) return;
//         allNodesInCircuit.push_back(node);
//         if (node->isGround()) {
//             if (groundNodeRef != nullptr && groundNodeRef != node) {
//                 cout << "هشدار: چندین گره زمین تعریف شده است. از اولین گره زمین شناسایی‌شده استفاده می‌شود: "
//                      << groundNodeRef->getName() << endl;
//             } else if (groundNodeRef == nullptr) {
//                 groundNodeRef = node;
//             }
//         }
//     }
//
//     // افزودن المان به مدار
//     void addElement(Element* element) {
//         if (!element) return;
//         elementsInCircuit.push_back(element);
//     }
//
//     // تنظیم صریح گره زمین
//     void setGroundNode(Node* gnd) {
//         if (!gnd) {
//             throw std::invalid_argument("گره زمین نمی‌تواند تهی (null) باشد.");
//         }
//         bool found = false;
//         for(Node* n : allNodesInCircuit) {
//             if(n == gnd) {
//                 found = true;
//                 break;
//             }
//         }
//         if(!found) addNode(gnd); // اگر گره زمین هنوز به لیست گره‌ها اضافه نشده، اضافه کن
//
//         groundNodeRef = gnd;
//     }
//
//     // تشکیل و دریافت ماتریس سیستم A در معادله Ax = Z
//     Eigen::MatrixXd getSystemMatrixA() {
//         buildNodeAndVoltageSourceMaps(); // اطمینان از به‌روز بودن نگاشت‌ها
//
//         int numNonGroundNodes = orderedNonGroundNodes.size();
//         int numVoltageSources = orderedVoltageSources.size();
//         int systemSize = numNonGroundNodes + numVoltageSources;
//
//         if (systemSize == 0) {
//             // اگر مداری خالی یا فقط با یک گره زمین باشد
//              throw std::runtime_error("خطا: مدار برای تحلیل بسیار کوچک است (فاقد گره‌های غیر زمین یا منابع ولتاژ).");
//         }
//
//         Eigen::MatrixXd A = Eigen::MatrixXd::Zero(systemSize, systemSize);
//
//         // بخش G (مربوط به هدایت‌ها - مقاومت‌ها)
//         for (Element* elem : elementsInCircuit) {
//             if (auto res = dynamic_cast<Resistor*>(elem)) {
//                 double conductance = 1.0 / res->getValue();
//                 Node* n1 = res->getNode1();
//                 Node* n2 = res->getNode2();
//
//                 // تاثیر بر قطر اصلی
//                 if (n1 != groundNodeRef) {
//                     A(nodeToIndexMap[n1], nodeToIndexMap[n1]) += conductance;
//                 }
//                 if (n2 != groundNodeRef) {
//                     A(nodeToIndexMap[n2], nodeToIndexMap[n2]) += conductance;
//                 }
//                 // تاثیر بر عناصر خارج از قطر
//                 if (n1 != groundNodeRef && n2 != groundNodeRef) {
//                     A(nodeToIndexMap[n1], nodeToIndexMap[n2]) -= conductance;
//                     A(nodeToIndexMap[n2], nodeToIndexMap[n1]) -= conductance;
//                 }
//             }
//         }
//
//         // بخش B و C (مربوط به منابع ولتاژ)
//         // طبق توضیحات فایل PDF بخش ۵.۲
//         for (size_t i = 0; i < orderedVoltageSources.size(); ++i) {
//             VoltageSource* vs = orderedVoltageSources[i];
//             Node* n_plus = vs->getNode1();  // گره مثبت منبع ولتاژ
//             Node* n_minus = vs->getNode2(); // گره منفی منبع ولتاژ
//             int vsMNAIndex = vsToIndexMap[vs]; // اندیس مجهول جریان این منبع ولتاژ
//
//             if (n_plus != groundNodeRef) {
//                 int nodeIdx = nodeToIndexMap[n_plus];
//                 A(nodeIdx, numNonGroundNodes + vsMNAIndex) += 1.0;  // بخش B
//                 A(numNonGroundNodes + vsMNAIndex, nodeIdx) += 1.0;  // بخش C
//             }
//             if (n_minus != groundNodeRef) {
//                 int nodeIdx = nodeToIndexMap[n_minus];
//                 A(nodeIdx, numNonGroundNodes + vsMNAIndex) -= 1.0; // بخش B
//                 A(numNonGroundNodes + vsMNAIndex, nodeIdx) -= 1.0; // بخش C
//             }
//         }
//
//         // بخش D ماتریس A برای منابع ولتاژ مستقل ایده‌آل صفر است.
//         // Eigen::MatrixXd::Zero قبلاً این بخش را صفر کرده است.
//
//         return A;
//     }
//
//     // تشکیل و دریافت بردار سمت راست Z (یا B در برخی منابع) در معادله Ax = Z
//     Eigen::VectorXd getSystemVectorZ() {
//         // فرض می‌شود buildNodeAndVoltageSourceMaps() قبلاً فراخوانی شده است
//         int numNonGroundNodes = orderedNonGroundNodes.size();
//         int numVoltageSources = orderedVoltageSources.size();
//         int systemSize = numNonGroundNodes + numVoltageSources;
//
//         if (systemSize == 0 && numNonGroundNodes == 0) { // بررسی مجدد برای حالت خاص بردار Z
//              // اگرچه getSystemMatrixA این را بررسی می‌کند، برای اطمینان
//              Eigen::VectorXd Z_empty(0);
//              return Z_empty;
//         }
//
//
//         Eigen::VectorXd Z = Eigen::VectorXd::Zero(systemSize);
//
//         // بخش J (مربوط به منابع جریان)
//         // طبق توضیحات فایل PDF بخش ۵.۲
//         for (Element* elem : elementsInCircuit) {
//             if (auto cs = dynamic_cast<CurrentSource*>(elem)) {
//                 Node* n_from = cs->getNode1(); // جریان از این گره خارج می‌شود
//                 Node* n_to = cs->getNode2();   // جریان به این گره وارد می‌شود
//                 double currentValue = cs->getValue();
//
//                 if (n_to != groundNodeRef) { // جریان ورودی به گره n_to
//                     Z(nodeToIndexMap[n_to]) += currentValue;
//                 }
//                 if (n_from != groundNodeRef) { // جریان خروجی از گره n_from
//                     Z(nodeToIndexMap[n_from]) -= currentValue;
//                 }
//             }
//         }
//
//         // بخش E (مربوط به مقادیر منابع ولتاژ)
//         // طبق توضیحات فایل PDF بخش ۵.۲
//         for (size_t i = 0; i < orderedVoltageSources.size(); ++i) {
//             VoltageSource* vs = orderedVoltageSources[i];
//             int vsMNAIndex = vsToIndexMap[vs];
//             Z(numNonGroundNodes + vsMNAIndex) = vs->getValue();
//         }
//         return Z;
//     }
//
//     // متدهای Getter برای دسترسی به لیست‌های داخلی (برای به‌روزرسانی وضعیت پس از حل)
//     const vector<Node*>& getOrderedNonGroundNodes() const {
//         return orderedNonGroundNodes;
//     }
//
//     const vector<VoltageSource*>& getOrderedVoltageSources() const {
//         return orderedVoltageSources;
//     }
//     // برای دسترسی به نگاشت vs->index برای بروزرسانی جریان vs ها
//      const map<VoltageSource*, int>& getVoltageSourceToIndexMap() const {
//         return vsToIndexMap;
//     }
//      const vector<Element*>& getAllElements() const { // برای محاسبه جریان مقاومت‌ها پس از حل
//         return elementsInCircuit;
//     }
// };
//
// // کلاس حل‌کننده سیستم معادلات MNA
// class MNASolver {
// public:
//     MNASolver() {}
//
//     // حل سیستم AX = Z و برگرداندن بردار X (شامل ولتاژ گره‌ها و سپس جریان منابع ولتاژ)
//     Eigen::VectorXd solve(const Eigen::MatrixXd& A, const Eigen::VectorXd& Z) {
//         if (A.rows() != A.cols() || A.rows() != Z.size()) {
//             throw std::runtime_error("خطا: ابعاد ماتریس و بردار برای حل سازگار نیستند.");
//         }
//         if (A.rows() == 0) {
//              throw std::runtime_error("خطا: سیستم معادلات برای حل خالی است.");
//         }
//
//         // استفاده از تجزیه LU با Partial Pivoting برای پایداری عددی
//         Eigen::PartialPivLU<Eigen::MatrixXd> lu(A);
//         if (!lu.isInvertible()) {
//             // این خطا می‌تواند برای مدارهای تعریف‌نشده (مثلاً بخش‌های شناور) یا ماتریس‌های تکین رخ دهد.
//             // مطابق فایل PDF، بخش ۱۹ (مدیریت خطاها - مدار ناپیوسته)
//             throw std::runtime_error("خطا: ماتریس سیستم تکین یا بدحالت است. مدار ممکن است قابل حل نباشد (مثلاً بخش‌های شناور، منابع ولتاژ اضافی و ...).");
//         }
//         return lu.solve(Z);
//     }
//
//     // به‌روزرسانی ولتاژ گره‌ها و جریان منابع ولتاژ پس از حل سیستم
//     void updateCircuitState(const Eigen::VectorXd& X, MakingMNA& mnaCircuit) {
//         const auto& nonGroundNodes = mnaCircuit.getOrderedNonGroundNodes();
//         const auto& voltageSources = mnaCircuit.getOrderedVoltageSources(); // این لیست مرتب است
//         const auto& vsMap = mnaCircuit.getVoltageSourceToIndexMap(); // برای گرفتن اندیس صحیح هر vs
//
//         int numNonGroundNodes = nonGroundNodes.size();
//
//         // به‌روزرسانی ولتاژ گره‌ها
//         for (int i = 0; i < numNonGroundNodes; ++i) {
//             nonGroundNodes[i]->setVoltage(X(i));
//         }
//
//         // به‌روزرسانی جریان عبوری از منابع ولتاژ
//         // ترتیب در orderedVoltageSources با ترتیب اندیس‌ها در vsMap مطابقت دارد
//         for (size_t i = 0; i < voltageSources.size(); ++i) {
//             VoltageSource* vs = voltageSources[i]; // vs فعلی از لیست مرتب شده
//             int vsMNAIndex = vsMap.at(vs); // اندیس این vs در بخش جریان‌های مجهول ماتریس MNA
//             vs->setCurrent(X(numNonGroundNodes + vsMNAIndex));
//         }
//     }
// };
//
//
// // تابع اصلی برای تست
// int main() {
//     // تنظیم دقت خروجی برای اعداد اعشاری
//     cout << fixed << setprecision(6);
//
//     // ۱. ایجاد گره‌ها
//     Node n1("1"), n2("2"), n_gnd("0"); // گره "0" یا "GND" معمولاً زمین است
//
//     // ۲. ایجاد مدیر مدار و افزودن گره‌ها
//     MakingMNA circuit;
//     circuit.addNode(&n1);
//     circuit.addNode(&n2);
//     circuit.addNode(&n_gnd);
//     // circuit.setGroundNode(&n_gnd); // تنظیم صریح گره زمین (اگرچه به طور خودکار نیز شناسایی می‌شود)
//
//     // ۳. ایجاد المان‌ها و افزودن آنها به مدار
//     try {
//         // مثال از فایل PDF بخش ۹ (مدار RC): V1=5V, R=1k, C=1uF (اینجا C را با مقاومت دوم جایگزین می‌کنیم برای مثال DC)
//         // V1 بین گره 1 و زمین، R1 بین 1 و 2، R2 بین 2 و زمین
//         VoltageSource vs(&n1, &n_gnd, "V1", 5.0);   // منبع ولتاژ 5 ولت بین گره 1 و زمین
//         Resistor r1(&n1, &n2, "R1", 1000.0);     // مقاومت 1 کیلواهم بین گره 1 و 2
//         Resistor r2(&n2, &n_gnd, "R2", 2000.0);   // مقاومت 2 کیلواهم بین گره 2 و زمین
//         // CurrentSource cs(&n_gnd, &n2, "I1", 0.001); // مثال: منبع جریان 1 میلی‌آمپر از زمین به گره 2
//
//         circuit.addElement(&vs);
//         circuit.addElement(&r1);
//         circuit.addElement(&r2);
//         // circuit.addElement(&cs);
//
//         // ۴. دریافت ماتریس‌های MNA
//         Eigen::MatrixXd A = circuit.getSystemMatrixA();
//         Eigen::VectorXd Z = circuit.getSystemVectorZ();
//
//         cout << "ماتریس سیستم A:\n" << A << endl << endl;
//         cout << "بردار سیستم Z:\n" << Z << endl << endl;
//
//         // ۵. حل سیستم
//         MNASolver solver;
//         Eigen::VectorXd X = solver.solve(A, Z);
//
//         cout << "بردار حل X (شامل ولتاژ گره‌ها و سپس جریان منابع ولتاژ):\n" << X << endl << endl;
//
//         // ۶. به‌روزرسانی ولتاژ گره‌ها و جریان منابع ولتاژ در اشیاء مربوطه
//         solver.updateCircuitState(X, circuit);
//
//         // ۷. چاپ نتایج
//         cout << "ولتاژ گره‌ها پس از حل:" << endl;
//         for (const auto* node : circuit.getOrderedNonGroundNodes()) {
//             cout << "گره " << node->getName() << ": " << node->getVoltage() << " V" << endl;
//         }
//
//         cout << "\nجریان عبوری از منابع ولتاژ:" << endl;
//         for (const auto* vs_elem : circuit.getOrderedVoltageSources()) {
//             cout << "جریان عبوری از " << vs_elem->getName() << ": " << vs_elem->getCurrent() << " A" << endl;
//         }
//
//         cout << "\nجریان عبوری از مقاومت‌ها (محاسبه‌شده پس از حل):" << endl;
//         for (const auto* elem : circuit.getAllElements()) {
//            if (const Resistor* res = dynamic_cast<const Resistor*>(elem)) {
//                cout << "جریان عبوری از " << res->getName() << " (" << res->getNode1()->getName() << "->" << res->getNode2()->getName() << "): "
//                     << res->getCurrent() << " A" << endl;
//            }
//         }
//
//
//     } catch (const std::exception& e) {
//         cerr << "یک خطا رخ داد: " << e.what() << endl;
//         // مثال‌هایی از پیام‌های خطا در فایل PDF:
//         // "خطا: مقدار مقاومت نمی‌تواند صفر یا منفی باشد."
//         // "خطا: گره زمین (گراند) در مدار تشخیص داده نشد."
//         // "خطا: مقدار منفی یا صفر برای یک المان نامعتبر است."
//         // "خطا: ماتریس سیستم تکین یا بدحالت است..."
//     }
//
//     return 0;
// }

////////////////////////////////////////////////
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept> // برای کلاس‌های استثنا مانند std::invalid_argument
#include <map>       // برای نگاشت گره‌ها به اندیس ماتریس
#include <iomanip>   // برای форматирование вывода (std::fixed, std::setprecision)
#include <cmath>     // برای std::abs و std::fabs
#include <algorithm> // برای std::swap

// استفاده از فضای نام std برای سادگی در این مثال
// در پروژه‌های بزرگتر، بهتر است از پیشوند std:: استفاده شود یا using declarations محدودتری به کار رود.
using namespace std;

// تعریف نوع برای ماتریس و بردار
using Matrix = vector<vector<double>>;
using Vector = vector<double>;

// تعریف کلاس گره (Node)
class Node {
private:
    double voltage; // ولتاژ گره، به double تغییر داده شد
    string name;    // نام گره

public:
    // سازنده کلاس گره
    Node(const string &name, double voltage = 0.0) { // مقدار پیش‌فرض ولتاژ 0.0 در نظر گرفته شد
        this->name = name;
        this->voltage = voltage;
    }

    // دریافت نام گره
    string getName() const {
        return name;
    }

    // دریافت ولتاژ گره
    double getVoltage() const {
        return voltage;
    }

    // تنظیم ولتاژ گره
    void setVoltage(double v) {
        voltage = v;
    }

    // بررسی اینکه آیا گره، گره زمین (گراند) است یا خیر
    bool isGround() const {
        return name == "0" || name == "GND" || name == "gnd"; // نام‌های رایج برای زمین
    }
};

// تعریف کلاس پایه برای المان‌های مداری (Element)
class Element {
protected:
    Node *node1, *node2; // اشاره‌گر به گره‌های متصل به المان
    string name;         // نام المان

public:
    // سازنده کلاس المان
    Element(Node* n1, Node* n2, const string &name) {
        if (!n1 || !n2) {
            throw std::invalid_argument("Nodes cannot be null.");
        }
        this->node1 = n1;
        this->node2 = n2;
        this->name = name;
    }

    // مخرب مجازی پیش‌فرض
    virtual ~Element() = default;

    // دریافت نام المان
    string getName() const { return name; }

    // دریافت گره اول
    Node* getNode1() const { return node1; }

    // دریافت گره دوم
    Node* getNode2() const { return node2; }

    // تابع مجازی خالص برای دریافت نوع المان (باید در کلاس‌های مشتق پیاده‌سازی شود)
    virtual string getType() const = 0;

    // تابع مجازی خالص برای دریافت مقدار مشخصه المان (مقاومت، ولتاژ منبع، جریان منبع و ...)
    virtual double getValue() const = 0;

    // تابع مجازی برای دریافت جریان عبوری از المان (می‌تواند در کلاس‌های مشتق بازنویسی شود)
    // برای المان‌هایی مانند مقاومت پس از حل مدار محاسبه می‌شود
    // برای منابع ولتاژ، این یکی از مجهولات سیستم MNA است
    virtual double getCurrent() const {
        return 0.0; // پیاده‌سازی پیش‌فرض
    }

    // تابع مجازی برای تنظیم جریان عبوری از المان (مربوط به منابع ولتاژ)
    virtual void setCurrent(double current) {
        // پیاده‌سازی پیش‌فرض، برای جلوگیری از هشدار پارامتر استفاده‌نشده
        (void)current;
    }
};

// کلاس مقاومت (Resistor) مشتق‌شده از Element
class Resistor : public Element {
private:
    double resistance; // مقدار مقاومت

public:
    Resistor(Node* n1, Node* n2, const string &name, double res) : Element(n1, n2, name) {
        if (res <= 0) {
            // مطابق فایل PDF، بخش ۱۹ (مدیریت خطاها)
            throw std::invalid_argument("Error:value of Resistor cannot be negative. Resistor Name: " + name);
        }
        this->resistance = res;
    }

    string getType() const override { return "Resistor"; }
    double getValue() const override { return resistance; } // مقدار مقاومت را برمی‌گرداند

    // جریان عبوری از مقاومت طبق قانون اهم (پس از مشخص شدن ولتاژ گره‌ها)
    double getCurrent() const override {
        if (node1 && node2) {
            return (node1->getVoltage() - node2->getVoltage()) / resistance;
        }
        return 0.0;
    }
};

// کلاس منبع ولتاژ مستقل (VoltageSource) مشتق‌شده از Element
class VoltageSource : public Element {
private:
    double voltageValue;         // مقدار ولتاژ منبع
    double currentThroughSource; // جریان عبوری از منبع (یکی از مجهولات MNA)

public:
    VoltageSource(Node* n1, Node* n2, const string &name, double val) : Element(n1, n2, name) {
        this->voltageValue = val;
        this->currentThroughSource = 0.0; // مقدار اولیه
    }

    string getType() const override { return "VoltageSource"; }
    double getValue() const override { return voltageValue; } // مقدار ولتاژ تنظیم‌شده منبع را برمی‌گرداند

    double getCurrent() const override {
        return currentThroughSource;
    }
    void setCurrent(double current) override {
        this->currentThroughSource = current;
    }
};

// کلاس منبع جریان مستقل (CurrentSource) مشتق‌شده از Element
class CurrentSource : public Element {
private:
    double currentValue; // مقدار جریان منبع

public:
    // گره n1 گرهی است که جریان از آن خارج می‌شود، گره n2 گرهی است که جریان به آن وارد می‌شود
    CurrentSource(Node* n1, Node* n2, const string &name, double val) : Element(n1, n2, name) {
        this->currentValue = val;
    }

    string getType() const override { return "CurrentSource"; }
    double getValue() const override { return currentValue; } // مقدار جریان تنظیم‌شده منبع را برمی‌گرداند

    // ولتاژ دو سر منبع جریان توسط مدار تعیین می‌شود و جزو مجهولات مستقیم برای این المان نیست
};


// کلاس مسئول تشکیل ماتریس‌های MNA
class MakingMNA {
private:
    vector<Node*> allNodesInCircuit;     // لیست تمام گره‌های موجود در مدار
    vector<Element*> elementsInCircuit;  // لیست تمام المان‌های موجود در مدار
    Node* groundNodeRef;                 // اشاره‌گر به گره زمین

    // نگاشت‌ها برای تشکیل ماتریس
    map<Node*, int> nodeToIndexMap;          // نگاشت گره‌های غیرزمین به اندیس سطر/ستون در ماتریس MNA
    vector<Node*> orderedNonGroundNodes;     // لیست مرتب‌شده گره‌های غیرزمین
    map<VoltageSource*, int> vsToIndexMap;   // نگاشت منابع ولتاژ به اندیس مجهول جریانشان در MNA
    vector<VoltageSource*> orderedVoltageSources; // لیست مرتب‌شده منابع ولتاژ

    // متد داخلی برای ساخت نگاشت‌ها
    void buildNodeAndVoltageSourceMaps() {
        nodeToIndexMap.clear();
        orderedNonGroundNodes.clear();
        vsToIndexMap.clear();
        orderedVoltageSources.clear();

        // تعیین گره زمین در صورت عدم تنظیم صریح
        if (!groundNodeRef) {
            for (Node* n : allNodesInCircuit) {
                if (n->isGround()) {
                    groundNodeRef = n;
                    break;
                }
            }
            if (!groundNodeRef) {
                // مطابق فایل PDF، بخش ۱۹ (مدیریت خطاها)
                throw std::runtime_error("Error: invable Ground Node. impossiple solving!");
            }
        }

        // شماره‌گذاری گره‌های غیرزمین
        int nodeIdx = 0;
        for (Node* node : allNodesInCircuit) {
            if (node != groundNodeRef) {
                orderedNonGroundNodes.push_back(node);
                nodeToIndexMap[node] = nodeIdx++;
            }
        }

        // شماره‌گذاری منابع ولتاژ (برای مجهولات جریان آنها)
        int vsIdx = 0;
        for (Element* elem : elementsInCircuit) {
            if (auto vs = dynamic_cast<VoltageSource*>(elem)) {
                orderedVoltageSources.push_back(vs);
                vsToIndexMap[vs] = vsIdx++;
            }
        }
    }

public:
    // سازنده
    MakingMNA() : groundNodeRef(nullptr) {}

    // مخرب (در صورت نیاز به مدیریت حافظه پیچیده‌تر، باید تکمیل شود)
    ~MakingMNA() {
        // اگر گره‌ها و المان‌ها در این کلاس new شده باشند، باید delete شوند.
        // در این پیاده‌سازی، فرض بر این است که مدیریت حافظه آنها خارج از این کلاس انجام می‌شود.
    }

    // افزودن گره به مدار
    void addNode(Node* node) {
        if (!node) return;
        allNodesInCircuit.push_back(node);
        if (node->isGround()) {
            if (groundNodeRef != nullptr && groundNodeRef != node) {
                cout << "Warning: there is some Ground Node. first Node has been used: "
                     << groundNodeRef->getName() << endl;
            } else if (groundNodeRef == nullptr) {
                groundNodeRef = node;
            }
        }
    }

    // افزودن المان به مدار
    void addElement(Element* element) {
        if (!element) return;
        elementsInCircuit.push_back(element);
    }

    // تنظیم صریح گره زمین
    void setGroundNode(Node* gnd) {
        if (!gnd) {
            throw std::invalid_argument("Ground Node cannot be null.");
        }
        bool found = false;
        for(Node* n : allNodesInCircuit) {
            if(n == gnd) {
                found = true;
                break;
            }
        }
        if(!found) addNode(gnd); // اگر گره زمین هنوز به لیست گره‌ها اضافه نشده، اضافه کن

        groundNodeRef = gnd;
    }

    // تشکیل و دریافت ماتریس سیستم A در معادله Ax = Z
    Matrix getSystemMatrixA() {
        buildNodeAndVoltageSourceMaps(); // اطمینان از به‌روز بودن نگاشت‌ها

        int numNonGroundNodes = orderedNonGroundNodes.size();
        int numVoltageSources = orderedVoltageSources.size();
        int systemSize = numNonGroundNodes + numVoltageSources;

        if (systemSize == 0) {
            // اگر مداری خالی یا فقط با یک گره زمین باشد
             throw std::runtime_error("Error: there is no eny Node.");
        }

        Matrix A(systemSize, Vector(systemSize, 0.0));

        // بخش G (مربوط به هدایت‌ها - مقاومت‌ها)
        for (Element* elem : elementsInCircuit) {
            if (auto res = dynamic_cast<Resistor*>(elem)) {
                double conductance = 1.0 / res->getValue();
                Node* n1 = res->getNode1();
                Node* n2 = res->getNode2();

                // تاثیر بر قطر اصلی
                if (n1 != groundNodeRef) {
                    A[nodeToIndexMap[n1]][nodeToIndexMap[n1]] += conductance;
                }
                if (n2 != groundNodeRef) {
                    A[nodeToIndexMap[n2]][nodeToIndexMap[n2]] += conductance;
                }
                // تاثیر بر عناصر خارج از قطر
                if (n1 != groundNodeRef && n2 != groundNodeRef) {
                    A[nodeToIndexMap[n1]][nodeToIndexMap[n2]] -= conductance;
                    A[nodeToIndexMap[n2]][nodeToIndexMap[n1]] -= conductance;
                }
            }
        }

        // بخش B و C (مربوط به منابع ولتاژ)
        // طبق توضیحات فایل PDF بخش ۵.۲
        for (size_t i = 0; i < orderedVoltageSources.size(); ++i) {
            VoltageSource* vs = orderedVoltageSources[i];
            Node* n_plus = vs->getNode1();  // گره مثبت منبع ولتاژ
            Node* n_minus = vs->getNode2(); // گره منفی منبع ولتاژ
            int vsMNAIndex = vsToIndexMap[vs]; // اندیس مجهول جریان این منبع ولتاژ

            if (n_plus != groundNodeRef) {
                int nodeIdx = nodeToIndexMap[n_plus];
                A[nodeIdx][numNonGroundNodes + vsMNAIndex] += 1.0;  // بخش B
                A[numNonGroundNodes + vsMNAIndex][nodeIdx] += 1.0;  // بخش C
            }
            if (n_minus != groundNodeRef) {
                int nodeIdx = nodeToIndexMap[n_minus];
                A[nodeIdx][numNonGroundNodes + vsMNAIndex] -= 1.0; // بخش B
                A[numNonGroundNodes + vsMNAIndex][nodeIdx] -= 1.0; // بخش C
            }
        }

        // بخش D ماتریس A برای منابع ولتاژ مستقل ایده‌آل صفر است.
        // ماتریس با صفرها مقداردهی اولیه شده است.

        return A;
    }

    // تشکیل و دریافت بردار سمت راست Z (یا B در برخی منابع) در معادله Ax = Z
    Vector getSystemVectorZ() {
        // فرض می‌شود buildNodeAndVoltageSourceMaps() قبلاً فراخوانی شده است
        int numNonGroundNodes = orderedNonGroundNodes.size();
        int numVoltageSources = orderedVoltageSources.size();
        int systemSize = numNonGroundNodes + numVoltageSources;

        if (systemSize == 0 && numNonGroundNodes == 0) {
             Vector Z_empty; // بردار خالی
             return Z_empty;
        }

        Vector Z(systemSize, 0.0);

        // بخش J (مربوط به منابع جریان)
        // طبق توضیحات فایل PDF بخش ۵.۲
        for (Element* elem : elementsInCircuit) {
            if (auto cs = dynamic_cast<CurrentSource*>(elem)) {
                Node* n_from = cs->getNode1(); // جریان از این گره خارج می‌شود
                Node* n_to = cs->getNode2();   // جریان به این گره وارد می‌شود
                double currentValue = cs->getValue();

                if (n_to != groundNodeRef) { // جریان ورودی به گره n_to
                    Z[nodeToIndexMap[n_to]] += currentValue;
                }
                if (n_from != groundNodeRef) { // جریان خروجی از گره n_from
                    Z[nodeToIndexMap[n_from]] -= currentValue;
                }
            }
        }

        // بخش E (مربوط به مقادیر منابع ولتاژ)
        // طبق توضیحات فایل PDF بخش ۵.۲
        for (size_t i = 0; i < orderedVoltageSources.size(); ++i) {
            VoltageSource* vs = orderedVoltageSources[i];
            int vsMNAIndex = vsToIndexMap[vs]; // این اندیس برای vs در orderedVoltageSources است
            Z[numNonGroundNodes + vsMNAIndex] = vs->getValue();
        }
        return Z;
    }

    // متدهای Getter برای دسترسی به لیست‌های داخلی (برای به‌روزرسانی وضعیت پس از حل)
    const vector<Node*>& getOrderedNonGroundNodes() const {
        return orderedNonGroundNodes;
    }

    const vector<VoltageSource*>& getOrderedVoltageSources() const {
        return orderedVoltageSources;
    }
    // برای دسترسی به نگاشت vs->index برای بروزرسانی جریان vs ها
     const map<VoltageSource*, int>& getVoltageSourceToIndexMap() const {
        return vsToIndexMap;
    }
     const vector<Element*>& getAllElements() const { // برای محاسبه جریان مقاومت‌ها پس از حل
        return elementsInCircuit;
    }
};

// کلاس حل‌کننده سیستم معادلات MNA
class MNASolver {
public:
    MNASolver() {}

    // حل سیستم AX = Z و برگرداندن بردار X (شامل ولتاژ گره‌ها و سپس جریان منابع ولتاژ)
    // پیاده‌سازی با حذف گاوسی و محورگیری جزئی
    Vector solve(Matrix A, Vector Z) { // کپی از A و Z چون در حین عملیات تغییر می‌کنند
        int n = A.size();
        if (n == 0 || A[0].size() != static_cast<size_t>(n) || Z.size() != static_cast<size_t>(n)) {
            throw std::runtime_error("Error: solving problem.");
        }

        const double EPS = 1e-9; // یک مقدار کوچک برای بررسی تکین بودن

        for (int i = 0; i < n; ++i) {
            // محورگیری جزئی: یافتن بزرگترین عنصر در ستون فعلی (از سطر i به پایین)
            int pivot_row = i;
            for (int k = i + 1; k < n; ++k) {
                if (std::abs(A[k][i]) > std::abs(A[pivot_row][i])) {
                    pivot_row = k;
                }
            }

            // جابجایی سطرها در A و Z
            if (pivot_row != i) {
                std::swap(A[i], A[pivot_row]);
                std::swap(Z[i], Z[pivot_row]);
            }

            // بررسی تکین بودن (اگر عنصر روی قطر صفر یا خیلی کوچک باشد)
            if (std::abs(A[i][i]) < EPS) {
                throw std::runtime_error("Error: solving problem for system.");
            }

            // نرمال‌سازی سطر i (عنصر قطری را 1 کن)
            // این مرحله اختیاری است اما می‌تواند به پایداری کمک کند، گرچه در اینجا مستقیماً برای حذف استفاده می‌کنیم.
            // برای سادگی، مستقیماً از A[i][i] به عنوان مقسوم‌علیه استفاده می‌کنیم.

            // حذف عناصر زیر قطر در ستون i
            for (int k = i + 1; k < n; ++k) {
                double factor = A[k][i] / A[i][i];
                for (int j = i; j < n; ++j) { // از ستون i شروع کن
                    A[k][j] -= factor * A[i][j];
                }
                Z[k] -= factor * Z[i];
            }
        }

        // جایگزینی پسرو (Back Substitution)
        Vector X(n);
        for (int i = n - 1; i >= 0; --i) {
            double sum = 0.0;
            for (int j = i + 1; j < n; ++j) {
                sum += A[i][j] * X[j];
            }
            if (std::abs(A[i][i]) < EPS) { // بررسی مجدد برای تقسیم بر صفر در جایگزینی پسرو
                 throw std::runtime_error("Error: solving problem for system.");
            }
            X[i] = (Z[i] - sum) / A[i][i];
        }
        return X;
    }


    // به‌روزرسانی ولتاژ گره‌ها و جریان منابع ولتاژ پس از حل سیستم
    void updateCircuitState(const Vector& X, MakingMNA& mnaCircuit) {
        const auto& nonGroundNodes = mnaCircuit.getOrderedNonGroundNodes();
        const auto& voltageSources = mnaCircuit.getOrderedVoltageSources();
        const auto& vsMap = mnaCircuit.getVoltageSourceToIndexMap();

        int numNonGroundNodes = nonGroundNodes.size();

        if (X.size() != numNonGroundNodes + voltageSources.size()) {
            throw std::runtime_error("Error: solving problem.");
        }

        // به‌روزرسانی ولتاژ گره‌ها
        for (size_t i = 0; i < static_cast<size_t>(numNonGroundNodes); ++i) {
            nonGroundNodes[i]->setVoltage(X[i]);
        }

        // به‌روزرسانی جریان عبوری از منابع ولتاژ
        for (size_t i = 0; i < voltageSources.size(); ++i) {
            VoltageSource* vs = voltageSources[i];
            // اندیس vs در بخش جریان‌های مجهول ماتریس MNA
            // vsMap کلیدش VoltageSource* و مقدارش اندیس اولیه است (0, 1, 2, ...)
            // این اندیس‌ها باید با ترتیب در orderedVoltageSources مطابقت داشته باشند.
            // بنابراین، اندیس در بردار X برابر است با numNonGroundNodes + (اندیس vs در orderedVoltageSources)
            // یا اگر vsMap اندیس صحیح را برای vs در X بدهد (یعنی vsMap[vs] اندیس نهایی در X باشد)
            // در پیاده‌سازی فعلی MakingMNA، vsMap اندیس‌های 0, 1, ... را برای vs ها برمی‌گرداند.
            int vsMNAIndex = vsMap.at(vs); // این اندیس 0, 1, ... برای منابع ولتاژ است
            vs->setCurrent(X[numNonGroundNodes + vsMNAIndex]);
        }
    }
};


// تابع کمکی برای چاپ ماتریس
void printMatrix(const Matrix& mat, const string& name) {
    cout << name << ":\n";
    if (mat.empty()) {
        cout << "(empty)\n";
        return;
    }
    for (const auto& row : mat) {
        for (double val : row) {
            cout << setw(10) << val << " ";
        }
        cout << endl;
    }
    cout << endl;
}

// تابع کمکی برای چاپ بردار
void printVector(const Vector& vec, const string& name) {
    cout << name << ":\n";
    if (vec.empty()) {
        cout << "(empty)\n";
        return;
    }
    for (double val : vec) {
        cout << setw(10) << val << " ";
    }
    cout << endl << endl;
}


// تابع اصلی برای تست
int main() {
    // تنظیم دقت خروجی برای اعداد اعشاری
    cout << fixed << setprecision(6);

    // ۱. ایجاد گره‌ها
    Node n1("1"), n2("2"), n_gnd("0"); // گره "0" یا "GND" معمولاً زمین است

    // ۲. ایجاد مدیر مدار و افزودن گره‌ها
    MakingMNA circuit;
    circuit.addNode(&n1);
    circuit.addNode(&n2);
    circuit.addNode(&n_gnd);
    // circuit.setGroundNode(&n_gnd); // تنظیم صریح گره زمین (اگرچه به طور خودکار نیز شناسایی می‌شود)

    // ۳. ایجاد المان‌ها و افزودن آنها به مدار
    try {
        // مثال از فایل PDF بخش ۹ (مدار RC): V1=5V, R=1k, C=1uF (اینجا C را با مقاومت دوم جایگزین می‌کنیم برای مثال DC)
        // V1 بین گره 1 و زمین، R1 بین 1 و 2، R2 بین 2 و زمین
        VoltageSource vs(&n1, &n_gnd, "V1", 5.0);   // منبع ولتاژ 5 ولت بین گره 1 و زمین
        Resistor r1(&n1, &n2, "R1", 1000.0);     // مقاومت 1 کیلواهم بین گره 1 و 2
        Resistor r2(&n2, &n_gnd, "R2", 2000.0);   // مقاومت 2 کیلواهم بین گره 2 و زمین
        // CurrentSource cs(&n_gnd, &n2, "I1", 0.001); // مثال: منبع جریان 1 میلی‌آمپر از زمین به گره 2

        circuit.addElement(&vs);
        circuit.addElement(&r1);
        circuit.addElement(&r2);
        // circuit.addElement(&cs);

        // ۴. دریافت ماتریس‌های MNA
        Matrix A = circuit.getSystemMatrixA();
        Vector Z = circuit.getSystemVectorZ();

        printMatrix(A, "A Matrix");
        printVector(Z, "Z Matrix");


        // ۵. حل سیستم
        MNASolver solver;
        Vector X = solver.solve(A, Z); // A و Z به صورت کپی ارسال می‌شوند

        printVector(X, "X vector");


        // ۶. به‌روزرسانی ولتاژ گره‌ها و جریان منابع ولتاژ در اشیاء مربوطه
        solver.updateCircuitState(X, circuit);

        // ۷. چاپ نتایج
        cout << "Voltage of Nodes after solving: " << endl;
        for (const auto* node : circuit.getOrderedNonGroundNodes()) {
            cout << "Node " << node->getName() << ": " << node->getVoltage() << " V" << endl;
        }

        cout << "\nCurrent Of voltage Source:" << endl;
        for (const auto* vs_elem : circuit.getOrderedVoltageSources()) {
            cout << "Current of " << vs_elem->getName() << ": " << vs_elem->getCurrent() << " A" << endl;
        }

        cout << "\nCurrent of Resistors: " << endl;
        for (const auto* elem : circuit.getAllElements()) {
           if (const Resistor* res = dynamic_cast<const Resistor*>(elem)) {
               cout << "Current of " << res->getName() << " (" << res->getNode1()->getName() << "->" << res->getNode2()->getName() << "): "
                    << res->getCurrent() << " A" << endl;
           }
        }


    } catch (const std::exception& e) {
        cerr << "OOps. " << e.what() << endl;
    }

    return 0;
}
