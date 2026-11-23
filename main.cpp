// Nakuru Fresh Produce Market - lightweight C++17 teaching prototype.
// Build: g++ -std=c++17 -O2 main.cpp -o market_app
// Linux socket API is used only for the tiny HTTP transport; business logic uses C++17.
#include <algorithm>
#include <arpa/inet.h>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <regex>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using namespace std;

string escapeJson(const string& s) {
    string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        if (c == '\n') { out += "\\n"; continue; }
        if (c == '\r') continue;
        out += c;
    }
    return out;
}
string jsonString(const string& s) { return "\"" + escapeJson(s) + "\""; }
string field(const string& body, const string& key) {
    regex re("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    smatch m; return regex_search(body, m, re) ? m[1].str() : "";
}
double numberField(const string& body, const string& key) {
    regex re("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    smatch m; return regex_search(body, m, re) ? stod(m[1].str()) : 0.0;
}
string nowDateTime() {
    time_t t = time(nullptr); tm* v = localtime(&t); char b[32];
    strftime(b, sizeof(b), "%Y-%m-%dT%H:%M", v); return b;
}
string today() { return nowDateTime().substr(0, 10); }
string addDays(int days) {
    time_t t = time(nullptr) + days * 86400; tm* v = localtime(&t); char b[16];
    strftime(b, sizeof(b), "%Y-%m-%d", v); return b;
}
string makeId(const string& prefix, int n) { ostringstream o; o << prefix << setw(3) << setfill('0') << n; return o.str(); }

// Abstraction: every reportable market object exposes the same interface.
class MarketEntity {
public:
    virtual string toJson() const = 0;
    virtual string getSummary() const = 0;
    virtual ~MarketEntity() = default;
};

// Encapsulation: private fields are changed through validating constructors/setters.
class ProduceBatch : public MarketEntity {
    string id, produce, arrivalDate, expiryDate;
    double received, remaining;
    double reorderLevel;
public:
    ProduceBatch(string i, string p, double q, string arrival, string expiry, double reorder)
        : id(move(i)), produce(move(p)), arrivalDate(move(arrival)), expiryDate(move(expiry)),
          received(q), remaining(q), reorderLevel(reorder) {
        if (q <= 0 || expiryDate <= arrivalDate) throw invalid_argument("Invalid batch quantity or dates");
    }
    void setRemaining(double q) { if (q < 0 || q > received) throw invalid_argument("Invalid remaining stock"); remaining = q; }
    string getId() const { return id; }
    string getProduce() const { return produce; }
    string getArrivalDate() const { return arrivalDate; }
    string getExpiryDate() const { return expiryDate; }
    double getRemaining() const { return remaining; }
    double getReceived() const { return received; }
    double getReorderLevel() const { return reorderLevel; }
    string toJson() const override {
        ostringstream o; o << "{\"id\":" << jsonString(id) << ",\"produce\":" << jsonString(produce)
          << ",\"arrivalDate\":" << jsonString(arrivalDate) << ",\"expiryDate\":" << jsonString(expiryDate)
          << ",\"received\":" << received << ",\"remaining\":" << remaining << ",\"reorderLevel\":" << reorderLevel << "}"; return o.str();
    }
    string getSummary() const override { return produce + " batch " + id + ": " + to_string((int)remaining) + " remaining"; }
};

class Person {
protected:
    string id, name, phone;
public:
    Person(string i, string n, string p) : id(move(i)), name(move(n)), phone(move(p)) {}
    virtual ~Person() = default;
    string getId() const { return id; }
    string getName() const { return name; }
    string getPhone() const { return phone; }
};
class Retailer : public Person, public MarketEntity {
    string deliveryZone;
public:
    Retailer(string i, string n, string p, string z) : Person(move(i), move(n), move(p)), deliveryZone(move(z)) {}
    string getZone() const { return deliveryZone; }
    string toJson() const override { return "{\"id\":"+jsonString(id)+",\"name\":"+jsonString(name)+",\"phone\":"+jsonString(phone)+",\"zone\":"+jsonString(deliveryZone)+"}"; }
    string getSummary() const override { return name + " - " + deliveryZone; }
};
class Driver : public Person {
    string vehicleReg;
public:
    Driver(string i, string n, string p, string reg) : Person(move(i), move(n), move(p)), vehicleReg(move(reg)) {}
    string getVehicleReg() const { return vehicleReg; }
};

class Order : public MarketEntity {
    string id, retailerId, channel, produce, createdAt, status, batchUsed;
    double quantity, unitPrice, total;
public:
    Order(string i, string r, string c, string p, double q, double price, string batch)
      : id(move(i)), retailerId(move(r)), channel(move(c)), produce(move(p)), createdAt(nowDateTime()),
        status("Confirmed"), batchUsed(move(batch)), quantity(q), unitPrice(price), total(q*price) {
        if (q <= 0 || (channel != "WhatsApp" && channel != "Phone")) throw invalid_argument("Invalid order");
    }
    string getId() const { return id; } string getRetailerId() const { return retailerId; }
    string getProduce() const { return produce; } string getChannel() const { return channel; }
    string getCreatedAt() const { return createdAt; } string getStatus() const { return status; }
    string getBatchUsed() const { return batchUsed; } double getQuantity() const { return quantity; }
    double getTotal() const { return total; } double getUnitPrice() const { return unitPrice; }
    void setStatus(const string& s) { status = s; }
    string toJson() const override { ostringstream o; o << "{\"id\":"<<jsonString(id)<<",\"retailerId\":"<<jsonString(retailerId)<<",\"channel\":"<<jsonString(channel)<<",\"produce\":"<<jsonString(produce)<<",\"quantity\":"<<quantity<<",\"unitPrice\":"<<unitPrice<<",\"total\":"<<total<<",\"createdAt\":"<<jsonString(createdAt)<<",\"status\":"<<jsonString(status)<<",\"batchUsed\":"<<jsonString(batchUsed)<<"}"; return o.str(); }
    string getSummary() const override { return id + " - " + produce + " x " + to_string((int)quantity); }
};
class Delivery : public MarketEntity {
    string id, orderId, driverId, scheduledDate, status;
public:
    Delivery(string i, string o, string d, string date) : id(move(i)), orderId(move(o)), driverId(move(d)), scheduledDate(move(date)), status("Scheduled") {}
    string getId() const { return id; } string getOrderId() const { return orderId; }
    string getDriverId() const { return driverId; } string getScheduledDate() const { return scheduledDate; }
    string getStatus() const { return status; }
    string toJson() const override { return "{\"id\":"+jsonString(id)+",\"orderId\":"+jsonString(orderId)+",\"driverId\":"+jsonString(driverId)+",\"scheduledDate\":"+jsonString(scheduledDate)+",\"status\":"+jsonString(status)+"}"; }
    string getSummary() const override { return id + " dispatches " + orderId + " on " + scheduledDate; }
};

// Polymorphism: one report function works with batches, retailers, orders and deliveries.
class ReportGenerator {
public:
    string render(const vector<MarketEntity*>& entities) const {
        string out = "[";
        for (size_t i=0; i<entities.size(); ++i) { if (i) out += ","; out += entities[i]->toJson(); }
        return out + "]";
    }
};

class InventoryManager {
    // Composition: InventoryManager owns all domain collections for this prototype.
    vector<ProduceBatch> batches;
    vector<Retailer> retailers;
    vector<Driver> drivers;
    vector<Order> orders;
    vector<Delivery> deliveries;
    map<string,double> prices;
    map<string,double> reorderLevels;
    int nextOrder=4, nextBatch=6, nextDelivery=3;
public:
    InventoryManager() { seed(); load(); }
    void seed() {
        prices={{"Tomatoes",90},{"Cabbages",65},{"Hass Avocados",180},{"Red Onions",120}};
        reorderLevels={{"Tomatoes",80},{"Cabbages",50},{"Hass Avocados",35},{"Red Onions",60}};
        retailers.emplace_back("R001","Lakeview Grocers","0712 345 678","Section 58");
        retailers.emplace_back("R002","Milimani Fresh Mart","0722 456 789","Milimani");
        retailers.emplace_back("R003","Lanet Retail Supplies","0701 222 333","Lanet");
        drivers.emplace_back("D001","Peter Mwangi","0711 111 222","KDA 412B");
        drivers.emplace_back("D002","Grace Wanjiku","0722 222 333","KDC 908M");
        drivers.emplace_back("D003","Daniel Kiptoo","0700 333 444","KDB 771A");
        batches.emplace_back("B001","Tomatoes",120,addDays(-2),addDays(1),80);
        batches.emplace_back("B002","Tomatoes",90,addDays(-1),addDays(3),80);
        batches.emplace_back("B003","Cabbages",75,addDays(-2),addDays(5),50);
        batches.emplace_back("B004","Hass Avocados",48,addDays(-1),addDays(2),35);
        batches.emplace_back("B005","Red Onions",110,addDays(-3),addDays(12),60);
        // A few confirmed sample orders make reports useful on first launch.
        orders.emplace_back("O001","R001","WhatsApp","Tomatoes",20,90,"B001"); batches[0].setRemaining(100);
        orders.emplace_back("O002","R002","Phone","Cabbages",10,65,"B003"); batches[2].setRemaining(65);
        orders.emplace_back("O003","R003","WhatsApp","Hass Avocados",8,180,"B004"); batches[3].setRemaining(40);
        deliveries.emplace_back("DEL001","O001","D001",today());
        deliveries.emplace_back("DEL002","O002","D002",addDays(1));
    }
    const vector<ProduceBatch>& getBatches() const { return batches; }
    const vector<Retailer>& getRetailers() const { return retailers; }
    const vector<Driver>& getDrivers() const { return drivers; }
    const vector<Order>& getOrders() const { return orders; }
    const vector<Delivery>& getDeliveries() const { return deliveries; }
    double available(const string& produce) const { double n=0; for (auto& b:batches) if(b.getProduce()==produce) n+=b.getRemaining(); return n; }
    vector<ProduceBatch*> fifo(const string& produce) {
        vector<ProduceBatch*> v; for(auto& b:batches) if(b.getProduce()==produce && b.getRemaining()>0) v.push_back(&b);
        sort(v.begin(),v.end(),[](auto a,auto b){ return a->getExpiryDate()<b->getExpiryDate(); }); return v;
    }
    string createOrder(const string& retailerId,const string& channel,const string& produce,double qty) {
        if(qty<=0) throw invalid_argument("Quantity must be greater than zero");
        bool retailerOk=false; for(auto& r:retailers) if(r.getId()==retailerId) retailerOk=true;
        if(!retailerOk) throw invalid_argument("Select a valid retailer");
        if(!prices.count(produce)) throw invalid_argument("Select a valid produce item");
        if(qty>available(produce)) throw invalid_argument("Insufficient stock. Available: "+to_string((int)available(produce)));
        auto batchesFifo=fifo(produce); double left=qty; string firstBatch;
        for(auto* b:batchesFifo) { if(left<=0) break; double take=min(left,b->getRemaining()); if(firstBatch.empty()) firstBatch=b->getId(); b->setRemaining(b->getRemaining()-take); left-=take; }
        string id=makeId("O",nextOrder++); orders.emplace_back(id,retailerId,channel,produce,qty,prices[produce],firstBatch);
        return id;
    }
    string intake(const string& produce,double qty,const string& arrival,const string& expiry) {
        if(!prices.count(produce)) throw invalid_argument("Select a valid produce item");
        if(qty<=0 || expiry<=arrival) throw invalid_argument("Quantity must be positive and expiry must be after arrival");
        string id=makeId("B",nextBatch++); batches.emplace_back(id,produce,qty,arrival,expiry,reorderLevels[produce]); return id;
    }
    string dispatch(const string& orderId,const string& driverId,const string& date) {
        bool orderOk=false, driverOk=false, already=false;
        for(auto& o:orders) if(o.getId()==orderId) orderOk=true;
        for(auto& d:drivers) if(d.getId()==driverId) driverOk=true;
        for(auto& d:deliveries) if(d.getOrderId()==orderId) already=true;
        if(!orderOk || !driverOk || already) throw invalid_argument("Choose an existing undispatched order and driver");
        string id=makeId("DEL",nextDelivery++); deliveries.emplace_back(id,orderId,driverId,date); return id;
    }
    vector<MarketEntity*> allEntities() {
        vector<MarketEntity*> v; for(auto& x:batches)v.push_back(&x); for(auto& x:retailers)v.push_back(&x); for(auto& x:orders)v.push_back(&x); for(auto& x:deliveries)v.push_back(&x); return v;
    }
    string batchesJson() { vector<MarketEntity*> v; for(auto& b:batches)v.push_back(&b); return ReportGenerator().render(v); }
    string retailersJson() { vector<MarketEntity*> v; for(auto& r:retailers)v.push_back(&r); return ReportGenerator().render(v); }
    string ordersJson() { vector<MarketEntity*> v; for(auto& o:orders)v.push_back(&o); return ReportGenerator().render(v); }
    string deliveriesJson() { vector<MarketEntity*> v; for(auto& d:deliveries)v.push_back(&d); return ReportGenerator().render(v); }
    string productsJson() const { string s="["; bool first=true; for(auto& p:prices){ if(!first)s+=","; first=false; s+="{\"name\":"+jsonString(p.first)+",\"price\":"+to_string(p.second)+",\"available\":"+to_string((int)available(p.first))+",\"reorderLevel\":"+to_string((int)reorderLevels.at(p.first))+"}";} return s+"]"; }
    string inventoryJson() {
        vector<const ProduceBatch*> sorted; for(auto& b:batches)sorted.push_back(&b);
        sort(sorted.begin(),sorted.end(),[](auto a,auto b){return a->getExpiryDate()<b->getExpiryDate();});
        string s="["; for(size_t i=0;i<sorted.size();++i){if(i)s+=",";s+=sorted[i]->toJson();}return s+"]";
    }
    string alertsJson() {
        string s="["; bool first=true;
        for(auto& b:batches) if(b.getRemaining()>0 && b.getExpiryDate()<=addDays(2)) { if(!first)s+=",";first=false;s+="{\"type\":\"Expiring\",\"batchId\":"+jsonString(b.getId())+",\"produce\":"+jsonString(b.getProduce())+",\"remaining\":"+to_string((int)b.getRemaining())+",\"expiryDate\":"+jsonString(b.getExpiryDate())+"}"; }
        for(auto& p:prices) if(available(p.first)<reorderLevels[p.first]) { if(!first)s+=",";first=false;s+="{\"type\":\"Low stock\",\"batchId\":\"—\",\"produce\":"+jsonString(p.first)+",\"remaining\":"+to_string((int)available(p.first))+",\"expiryDate\":\"—\"}"; }
        return s+"]";
    }
    string manifestJson() {
        string s="["; bool first=true;
        for(auto& d:deliveries) { const Order* o=nullptr; const Retailer* r=nullptr; const Driver* driver=nullptr;
            for(auto& x:orders)if(x.getId()==d.getOrderId())o=&x;
            if(o){for(auto& x:retailers)if(x.getId()==o->getRetailerId())r=&x;}
            for(auto& x:drivers)if(x.getId()==d.getDriverId())driver=&x;
            if(!o||!r||!driver)continue; if(!first)s+=",";first=false;
            s+="{\"deliveryId\":"+jsonString(d.getId())+",\"orderId\":"+jsonString(o->getId())+",\"retailer\":"+jsonString(r->getName())+",\"zone\":"+jsonString(r->getZone())+",\"produce\":"+jsonString(o->getProduce())+",\"quantity\":"+to_string((int)o->getQuantity())+",\"batchUsed\":"+jsonString(o->getBatchUsed())+",\"driver\":"+jsonString(driver->getName())+",\"vehicleReg\":"+jsonString(driver->getVehicleReg())+",\"scheduledDate\":"+jsonString(d.getScheduledDate())+",\"status\":"+jsonString(d.getStatus())+"}";
        } return s+"]";
    }
    string salesJson() {
        map<string,double> totals; double grand=0; for(auto& o:orders) if(o.getCreatedAt().substr(0,10)==today()){totals[o.getProduce()]+=o.getTotal();grand+=o.getTotal();}
        string s="{\"grandTotal\":"+to_string((int)grand)+",\"byProduce\":["; bool first=true;
        for(auto& x:totals){if(!first)s+=",";first=false;s+="{\"produce\":"+jsonString(x.first)+",\"revenue\":"+to_string((int)x.second)+"}";} return s+"]}";
    }
    string dashboardJson() {
        int low=0,exp=0; for(auto& p:prices)if(available(p.first)<reorderLevels[p.first])low++;
        for(auto& b:batches)if(b.getRemaining()>0 && b.getExpiryDate()<=addDays(2))exp++;
        int todayOrders=0; double revenue=0; for(auto& o:orders)if(o.getCreatedAt().substr(0,10)==today()){todayOrders++;revenue+=o.getTotal();}
        return "{\"todayOrders\":"+to_string(todayOrders)+",\"dailyRevenue\":"+to_string((int)revenue)+",\"lowStockCount\":"+to_string(low)+",\"expiringBatches\":"+to_string(exp)+"}";
    }
    // Load the last saved snapshot. Retailers and drivers remain seeded reference data.
    void load() {
        ifstream f("data/market.csv"); if(!f) return;
        string line; getline(f,line); vector<ProduceBatch> loadedBatches; vector<Order> loadedOrders; vector<Delivery> loadedDeliveries;
        while(getline(f,line)) {
            stringstream ss(line); vector<string> c; string part; while(getline(ss,part,',')) c.push_back(part);
            try {
                if(c.size()>=8 && c[0]=="BATCH") { ProduceBatch b(c[1],c[2],stod(c[3]),c[4],c[5],stod(c[7])); b.setRemaining(stod(c[6])); loadedBatches.push_back(b); }
                else if(c.size()>=9 && c[0]=="ORDER") { Order o(c[1],c[2],c[3],c[4],stod(c[5]),stod(c[6]),c[7]); if(c[8]!="Confirmed")o.setStatus(c[8]); loadedOrders.push_back(o); }
                else if(c.size()>=6 && c[0]=="DELIVERY") loadedDeliveries.emplace_back(c[1],c[2],c[3],c[4]);
            } catch(...) { /* Ignore a malformed demo row and keep the remaining records. */ }
        }
        if(!loadedBatches.empty()) batches=move(loadedBatches);
        if(!loadedOrders.empty()) orders=move(loadedOrders);
        if(!loadedDeliveries.empty()) deliveries=move(loadedDeliveries);
        nextBatch=static_cast<int>(batches.size())+1; nextOrder=static_cast<int>(orders.size())+1; nextDelivery=static_cast<int>(deliveries.size())+1;
    }
    // CSV snapshot persistence keeps the prototype dependency-free and survives restarts.
    void save() const {
        ofstream f("data/market.csv"); if(!f)return;
        f<<"TYPE,id,ref,quantity,field1,field2,field3,field4,field5\n";
        for(auto& b:batches)f<<"BATCH,"<<b.getId()<<","<<b.getProduce()<<","<<b.getReceived()<<","<<b.getArrivalDate()<<","<<b.getExpiryDate()<<","<<b.getRemaining()<<","<<b.getReorderLevel()<<",-\n";
        for(auto& o:orders)f<<"ORDER,"<<o.getId()<<","<<o.getRetailerId()<<","<<o.getQuantity()<<","<<o.getChannel()<<","<<o.getProduce()<<","<<o.getUnitPrice()<<","<<o.getBatchUsed()<<","<<o.getStatus()<<"\n";
        for(auto& d:deliveries)f<<"DELIVERY,"<<d.getId()<<","<<d.getOrderId()<<","<<d.getDriverId()<<","<<d.getScheduledDate()<<",-\n";
    }
};

string readFile(const string& path) { ifstream f(path); if(!f)return ""; stringstream s;s<<f.rdbuf();return s.str(); }
string contentType(const string& path) { return path.size()>=5 && path.substr(path.size()-5)==".html" ? "text/html; charset=utf-8" : "text/plain; charset=utf-8"; }
void sendResponse(int client,int status,const string& type,const string& body,const string& extra="") {
    string reason=status==200?"OK":status==201?"Created":status==400?"Bad Request":status==404?"Not Found": "Error";
    ostringstream o;o<<"HTTP/1.1 "<<status<<" "<<reason<<"\r\nContent-Type: "<<type<<"\r\nContent-Length: "<<body.size()<<"\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Content-Type\r\nAccess-Control-Allow-Methods: GET, POST, OPTIONS\r\nConnection: close\r\n"<<extra<<"\r\n"<<body;
    string out=o.str();send(client,out.c_str(),out.size(),0);
}
void handleClient(int client, InventoryManager& market) {
    string req; char buf[8192]; ssize_t n;
    while((n=recv(client,buf,sizeof(buf),0))>0){req.append(buf,n); if(req.find("\r\n\r\n")!=string::npos){size_t h=req.find("\r\n\r\n");smatch m;regex cr("Content-Length: ([0-9]+)",regex::icase);size_t len=regex_search(req,m,cr)?stoul(m[1].str()):0;if(req.size()>=h+4+len)break;}}
    size_t split=req.find("\r\n\r\n"); if(split==string::npos){sendResponse(client,400,"application/json","{\"error\":\"Bad request\"}");return;}
    string head=req.substr(0,split),body=req.substr(split+4); istringstream hs(head);string method,path,version;hs>>method>>path>>version;
    try {
        if(method=="OPTIONS"){sendResponse(client,200,"text/plain","ok");return;}
        if(method=="GET" && (path=="/"||path=="/index.html")){string html=readFile("index.html");if(html.empty())html="<h1>Missing index.html</h1>";sendResponse(client,200,"text/html; charset=utf-8",html);return;}
        if(method=="GET") {
            if(path=="/api/dashboard")sendResponse(client,200,"application/json",market.dashboardJson());
            else if(path=="/api/inventory")sendResponse(client,200,"application/json",market.inventoryJson());
            else if(path=="/api/products")sendResponse(client,200,"application/json",market.productsJson());
            else if(path=="/api/retailers")sendResponse(client,200,"application/json",market.retailersJson());
            else if(path=="/api/orders")sendResponse(client,200,"application/json",market.ordersJson());
            else if(path=="/api/drivers") { sendResponse(client,200,"application/json","[{\"id\":\"D001\",\"name\":\"Peter Mwangi\",\"vehicleReg\":\"KDA 412B\"},{\"id\":\"D002\",\"name\":\"Grace Wanjiku\",\"vehicleReg\":\"KDC 908M\"},{\"id\":\"D003\",\"name\":\"Daniel Kiptoo\",\"vehicleReg\":\"KDB 771A\"}]"); }
            else if(path=="/api/manifest")sendResponse(client,200,"application/json",market.manifestJson());
            else if(path=="/api/alerts")sendResponse(client,200,"application/json",market.alertsJson());
            else if(path=="/api/sales")sendResponse(client,200,"application/json",market.salesJson());
            else sendResponse(client,404,"application/json","{\"error\":\"Not found\"}");
            return;
        }
        if(method=="POST") {
            string id;
            if(path=="/api/orders") id=market.createOrder(field(body,"retailerId"),field(body,"channel"),field(body,"produce"),numberField(body,"quantity"));
            else if(path=="/api/batches") id=market.intake(field(body,"produce"),numberField(body,"quantity"),field(body,"arrivalDate"),field(body,"expiryDate"));
            else if(path=="/api/dispatch") id=market.dispatch(field(body,"orderId"),field(body,"driverId"),field(body,"scheduledDate"));
            else {sendResponse(client,404,"application/json","{\"error\":\"Not found\"}");return;}
            market.save();sendResponse(client,201,"application/json","{\"id\":"+jsonString(id)+",\"message\":\"Saved successfully\"}");return;
        }
        sendResponse(client,404,"application/json","{\"error\":\"Not found\"}");
    } catch(const exception& e){sendResponse(client,400,"application/json","{\"error\":"+jsonString(e.what())+"}");}
}
int main() {
    mkdir("data",0755);
    InventoryManager market;
    int server=socket(AF_INET,SOCK_STREAM,0); if(server<0){cerr<<"Could not create server socket\n";return 1;}
    int yes=1;setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
    sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=INADDR_ANY;int port=8080; if(const char* envPort=getenv("PORT")) port=stoi(envPort); addr.sin_port=htons(port);
    if(bind(server,(sockaddr*)&addr,sizeof(addr))<0||listen(server,20)<0){cerr<<"Could not bind port 8080. Is it already in use?\n";close(server);return 1;}
    cout<<"Nakuru Fresh Produce Market listening on port "<<port<<"\nPress Ctrl+C to stop.\n";
    while(true){int client=accept(server,nullptr,nullptr);if(client>=0){handleClient(client,market);close(client);}}
    close(server);return 0;
}
