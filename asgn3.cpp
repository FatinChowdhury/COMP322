#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <exception>
#include <algorithm>


using namespace std;

class FileNotFoundException : public std::exception {
    public:
        const char* what() const noexcept override {
            return "File not found.";
        }
    };
    
    class InvalidDomainException : public std::exception {
    public:
        const char* what() const noexcept override {
            return "Invalid domain name.";
        }
    };
    
    class CacheFullException : public std::exception {
    public:
        const char* what() const noexcept override {
            return "Cache is full.";
        }
    };
    class CacheEmptyException : public std::exception {
    public:
        const char* what() const noexcept override {
            return "Cache is empty.";
        }
    };


class DNSManager {
public:
// As for the comments, then most of them will be in the second class
// since most things here are already explained in the first assignment's file
    virtual string get_ip_address_from_file(const string& domain_name) {
        ifstream dnsFile("dns.txt", ios::in);
        if (!dnsFile.is_open()) {
            throw FileNotFoundException();
        }

        string line;
        while (getline(dnsFile, line)) {
            size_t pos = line.find('=');
            if (pos == string::npos) {
                continue;
            }
            string domain = line.substr(0, pos);
            string ip = line.substr(pos + 1);
            if (domain == domain_name) {
                dnsFile.close();
                return ip;
            }
        }
        dnsFile.close();
        throw InvalidDomainException();
    }

    virtual void add_update_dns_file(const string& domain, const string& ip) {
        if (domain.empty() || ip.empty()) {
            throw InvalidDomainException();
        }
        ifstream dnsFile("dns.txt", ios::in);
        if (!dnsFile.is_open()) {
            throw FileNotFoundException();
        }
        ofstream tempFile("temp.txt", ios::out);
        string line;
        bool found = false;
        while (getline(dnsFile, line)) {
            size_t pos = line.find('=');
            if (pos == string::npos) {
                tempFile << line << endl;
                continue;
            }
            string domain_in_file = line.substr(0, pos);
            if (domain_in_file == domain) {
                tempFile << domain << "=" << ip << endl;
                found = true;
            } else {
                tempFile << line << endl;
            }
        }
        if (!found) {
            tempFile << domain << "=" << ip << endl;
        }
        dnsFile.close();
        tempFile.close();
        remove("dns.txt");
        rename("temp.txt", "dns.txt");
    }

    void print_dns_file(const string& filename) {
        ifstream dnsFile(filename, ios::in);
        if (!dnsFile.is_open()) {
            throw FileNotFoundException();
        }

        string line;
        while (getline(dnsFile, line)) {
            cout << line << endl;
        }
    }
};

class DistributedDNSManager : public DNSManager {
    protected:
        vector<string> dns_files = {"dns1.txt", "dns2.txt", "dns3.txt"};
    
        // Determine which file a domain belongs to based on first letter
        virtual string get_target_filename(const string& domain) {
            if (domain.empty()) return dns_files[0];
            
            char first_char = toupper(domain[0]);
            if (first_char >= 'A' && first_char <= 'I') return dns_files[0];
            if (first_char >= 'J' && first_char <= 'R') return dns_files[1];
            return dns_files[2]; // S-Z and others
        }
    
        // Get all DNS files that might contain the domain (for thorough searching)
        virtual vector<string> get_relevant_files(const string& domain) {
            string target = get_target_filename(domain);
            if (target == dns_files[0]) return {dns_files[0]};
            if (target == dns_files[1]) return {dns_files[1]};
            return {dns_files[2]};
        }
    
        // Helper function to process a single DNS file
        bool process_single_file(const string& filename, const string& domain, string& ip, 
                               bool for_lookup, const string& new_ip = "") {
            ifstream inFile(filename);
            ofstream tempFile("temp.txt");
            bool found = false;
            string line;
    
            while (getline(inFile, line)) {
                size_t pos = line.find('=');
                if (pos == string::npos) {
                    tempFile << line << endl;
                    continue;
                }
    
                string current_domain = line.substr(0, pos);
                string current_ip = line.substr(pos + 1);
    
                if (current_domain == domain) {
                    found = true;
                    if (for_lookup) {
                        ip = current_ip;
                    } else {
                        // For update/delete operations
                        if (!new_ip.empty()) {
                            tempFile << domain << "=" << new_ip << endl;
                        }
                        // If new_ip is empty, we're deleting, so don't write this line
                    }
                } else {
                    tempFile << line << endl;
                }
            }
    
            inFile.close();
            tempFile.close();
    
            // Replace the original file with the updated one
            remove(filename.c_str());
            rename("temp.txt", filename.c_str());
    
            return found;
        }
    
    public:
        // Override get_ip_address_from_file to check the correct distributed file
        string get_ip_address_from_file(const string& domain_name) override {
            string ip;
            vector<string> files_to_check = get_relevant_files(domain_name);
    
            for (const auto& file : files_to_check) {
                if (process_single_file(file, domain_name, ip, true)) {
                    return ip;
                }
            }
            return "";
        }
    
        // Override add_update_dns_file to update the correct distributed file
        void add_update_dns_file(const string& domain, const string& ip) override {
            string target_file = get_target_filename(domain);
            string dummy_ip;
            process_single_file(target_file, domain, dummy_ip, false, ip);
        }
    
        // New method to remove a DNS entry
        void remove_dns_entry(const string& domain) {
            string target_file = get_target_filename(domain);
            string dummy_ip;
            process_single_file(target_file, domain, dummy_ip, false);
        }
    
        // Print all DNS entries across all files
        void print_all_dns_entries() {
            cout << "=== Distributed DNS Entries ===" << endl;
            for (const auto& file : dns_files) {
                cout << "\nFile: " << file << endl;
                print_dns_file(file);
            }
        }
    
        // Initialize empty DNS files if they don't exist
        void initialize_files() {
            for (const auto& file : dns_files) {
                ofstream outFile(file);
                outFile.close();
            }
        }
    };
    class EnhancedDistributedDNSManager : public DistributedDNSManager {
        protected:
            // Override the method to use hash-based distribution
            const string get_target_filename(const string& domain) const override {
                if (domain.empty()) {
                    throw InvalidDomainException();
                }
        
                // Use std::hash to determine the file index
                size_t hash = std::hash<string>{}(domain);
                int file_index = hash % dns_files.size(); // Distribute across the available files
        
                // Return the corresponding DNSFile object
                return dns_files[file_index];
            }
        
        public:
            EnhancedDistributedDNSManager() {
                cout << "Using hash-based distribution for DNS entries:" << endl;
                cout << "- Domains are distributed across files using a hash function" << endl;
                cout << "- This ensures even distribution regardless of domain names" << endl;
                cout << "- No single file will become disproportionately large" << endl;
                cout << "- Lookup performance remains O(1) on average" << endl;
            }
        
            // Override print method to show distribution info
            void print_distribution_stats() const {
                vector<int> counts(dns_files.size(), 0);
                int total = 0;
        
                // Iterate over the files and count entries
                for (size_t i = 0; i < dns_files.size(); i++) {
                    auto entries = read_entries(dns_files[i].filename);
                    counts[i] = entries.size();
                    total += counts[i];
                }
        
                // Print the distribution statistics
                cout << "\nDNS File Distribution Statistics:" << endl;
                for (size_t i = 0; i < dns_files.size(); i++) {
                    double percent = total > 0 ? (counts[i] * 100.0 / total) : 0;
                    cout << dns_files[i].filename << ": " << counts[i]
                         << " entries (" << percent << "%)" << endl;
                }
            }
        };

class CacheManager {
    protected:
        struct Node {
            string domain;
            string ip;
            Node* prev;
            Node* next;
            Node(const string& d, const string& i) : domain(d), ip(i), prev(nullptr), next(nullptr) {}
            virtual ~Node() {} // Virtual destructor for proper cleanup in derived classes
        };
    
        DNSManager dnsManager;
        Node* head;
        Node* tail;
        int max_cache_size;
        int current_size;
    
        // Find node by traversing the list
        Node* find_node(const string& domain) {
            Node* current = head;
            while (current) {
                if (current->domain == domain) {
                    return current;
                }
                current = current->next;
            }
            return nullptr;
        }
    
        // Move node to front (for LRU)
        virtual void move_to_front(Node* node) {
            if (node == head) return;
            
            // Remove from current position
            if (node->prev) node->prev->next = node->next;
            if (node->next) node->next->prev = node->prev;
            if (node == tail) tail = node->prev;
            
            // Add to front
            node->prev = nullptr;
            node->next = head;
            if (head) head->prev = node;
            head = node;
            if (!tail) tail = head;
        }
    
        // Add new node to front
        void add_to_front(Node* node) {
            if (!head) {
                head = tail = node;
            } else {
                node->next = head;
                head->prev = node;
                head = node;
            }
            current_size++;
        }
    
        // Eviction policy (to be overridden by derived classes)
        virtual void evict() {
            if (!tail) return;
            
            // Remove from tail (LRU default)
            Node* prev = tail->prev;
            if (prev) prev->next = nullptr;
            delete tail;
            tail = prev;
            if (!tail) head = nullptr;
            current_size--;
        }
    
    public:
        CacheManager(int max_size) : head(nullptr), tail(nullptr), max_cache_size(max_size), current_size(0) {}
    
        virtual ~CacheManager() {
            Node* current = head;
            while (current) {
                Node* next = current->next;
                delete current;
                current = next;
            }
        }
    
        virtual string get_ip_address(const string& domain_name) {
            if (domain_name.empty()) {
                throw InvalidDomainException();
            }
            // Check if domain is in cache
            Node* node = find_node(domain_name);
            if (node) {
                move_to_front(node);
                return node->ip;
            }
    
            string ip = dnsManager.get_ip_address_from_file(domain_name);
            if (ip.empty()) {
                throw InvalidDomainException();
            }
    
            Node* new_node = create_node(domain_name, ip);
            if (current_size == max_cache_size) {
                evict();
            }
    
            add_to_front(new_node);
            return ip;
        }
    
        virtual void add_update_cache(const string& domain, const string& ip) {
            Node* node = find_node(domain);
            if (node) {
                node->ip = ip;
                move_to_front(node);
            } else {
                Node* new_node = create_node(domain, ip);
                if (current_size == max_cache_size) {
                    evict();
                }
                add_to_front(new_node);
            }
            dnsManager.add_update_dns_file(domain, ip);
        }
    
        virtual Node* create_node(const string& domain, const string& ip) {
            return new Node(domain, ip);
        }
    
        void print_cache() {
            if (!head) {
                throw CacheEmptyException();
            }
            Node* current = head;
            while (current) {
                cout << current->domain << " : " << current->ip << endl;
                current = current->next;
            }
        }
    };
    
    // LFU Cache Implementation
    class LFUCacheManager : public CacheManager {
    protected:
        struct LFUNode : public Node {
            int frequency;
            LFUNode(const string& d, const string& i) : Node(d, i), frequency(0) {}
        };
    
        void increment_frequency(Node* node) {
            static_cast<LFUNode*>(node)->frequency++;
        }
    
        void evict() override {
            if (!head) throw CacheEmptyException();
            
            // Find node with minimum frequency
            Node* to_evict = head;
            int min_freq = static_cast<LFUNode*>(to_evict)->frequency;
            
            Node* current = head->next;
            while (current) {
                int current_freq = static_cast<LFUNode*>(current)->frequency;
                if (current_freq < min_freq) {
                    min_freq = current_freq;
                    to_evict = current;
                }
                current = current->next;
            }
            
            // Remove the node
            if (to_evict->prev) to_evict->prev->next = to_evict->next;
            if (to_evict->next) to_evict->next->prev = to_evict->prev;
            if (to_evict == head) head = to_evict->next;
            if (to_evict == tail) tail = to_evict->prev;
            
            delete to_evict;
            current_size--;
        }
    
    public:
        LFUCacheManager(int max_size) : CacheManager(max_size) {}
    
        string get_ip_address(const string& domain_name) override {
            Node* node = find_node(domain_name);
            if (node) {
                increment_frequency(node);
                move_to_front(node);
                return node->ip;
            }
            if (domain_name.empty()) {
                throw InvalidDomainException();
            }
    
            string ip = dnsManager.get_ip_address_from_file(domain_name);
            if (ip.empty()) {
                throw InvalidDomainException();
            }
    
            LFUNode* new_node = new LFUNode(domain_name, ip);
            if (current_size == max_cache_size) {
                evict();
            }
    
            add_to_front(new_node);
            return ip;
        }
    
        Node* create_node(const string& domain, const string& ip) override {
            return new LFUNode(domain, ip);
        }
    };
    
    // LIFO Cache Implementation
    class LIFOCacheManager : public CacheManager {
    protected:
        void evict() override {
            if (!head) throw CacheEmptyException();
            
            // Always evict the head (most recently added)
            Node* to_evict = head;
            head = head->next;
            if (head) head->prev = nullptr;
            else tail = nullptr;
            
            delete to_evict;
            current_size--;
        }
    
        // Don't move to front on access for LIFO
        void move_to_front(Node* /*node*/) override {}
    
    public:
        LIFOCacheManager(int max_size) : CacheManager(max_size) {}
    
        string get_ip_address(const string& domain_name) override {
            Node* node = find_node(domain_name);
            if (node) {
                return node->ip; // No movement in LIFO
            }
            if (domain_name.empty()) {
                throw InvalidDomainException();
            }
    
            string ip = dnsManager.get_ip_address_from_file(domain_name);
            if (ip.empty()) {
                throw InvalidDomainException();
            }
    
            Node* new_node = create_node(domain_name, ip);
            if (current_size == max_cache_size) {
                evict();
            }
    
            // New items always added to front
            add_to_front(new_node);
            return ip;
        }
    };
    




    int main() {
        try {
            cout << "=== Distributed DNS Manager (A-I, J-R, S-Z) ===" << endl;
            DistributedDNSManager distributedDnsManager;
    
            // Add DNS entries
            cout << "\nAdding DNS entries to DistributedDNSManager..." << endl;
            distributedDnsManager.add_update_dns_file("apple.com", "1.1.1.1");
            distributedDnsManager.add_update_dns_file("ibm.com", "1.1.1.2");
            distributedDnsManager.add_update_dns_file("jetbrains.com", "2.2.2.1");
            distributedDnsManager.add_update_dns_file("microsoft.com", "2.2.2.2");
            distributedDnsManager.add_update_dns_file("sony.com", "3.3.3.1");
            distributedDnsManager.add_update_dns_file("zoom.com", "3.3.3.2");
    
            // Lookup DNS entries
            cout << "\nLooking up DNS entries in DistributedDNSManager..." << endl;
            cout << "apple.com: " << distributedDnsManager.get_ip_address_from_file("apple.com") << endl;
            cout << "microsoft.com: " << distributedDnsManager.get_ip_address_from_file("microsoft.com") << endl;
    
            // Print all DNS entries
            cout << "\nPrinting all DNS entries in DistributedDNSManager..." << endl;
            distributedDnsManager.print_all_entries();
    
            // Remove a DNS entry
            cout << "\nRemoving DNS entry for ibm.com in DistributedDNSManager..." << endl;
            distributedDnsManager.remove_dns_entry("ibm.com");
            try {
                cout << "ibm.com: " << distributedDnsManager.get_ip_address_from_file("ibm.com") << endl;
            } catch (const InvalidDomainException& e) {
                cout << "Error: " << e.what() << endl;
            }
    
            // Print all DNS entries after removal
            cout << "\nPrinting all DNS entries in DistributedDNSManager after removal..." << endl;
            distributedDnsManager.print_all_entries();
    
            cout << "\n=== Enhanced Distributed DNS Manager (Hash-Based) ===" << endl;
            EnhancedDistributedDNSManager enhancedDnsManager;
    
            // Add DNS entries
            cout << "\nAdding DNS entries to EnhancedDistributedDNSManager..." << endl;
            enhancedDnsManager.add_update_dns_file("apple.com", "1.1.1.1");
            enhancedDnsManager.add_update_dns_file("ibm.com", "1.1.1.2");
            enhancedDnsManager.add_update_dns_file("jetbrains.com", "2.2.2.1");
            enhancedDnsManager.add_update_dns_file("microsoft.com", "2.2.2.2");
            enhancedDnsManager.add_update_dns_file("sony.com", "3.3.3.1");
            enhancedDnsManager.add_update_dns_file("zoom.com", "3.3.3.2");
    
            // Lookup DNS entries
            cout << "\nLooking up DNS entries in EnhancedDistributedDNSManager..." << endl;
            cout << "apple.com: " << enhancedDnsManager.get_ip_address_from_file("apple.com") << endl;
            cout << "microsoft.com: " << enhancedDnsManager.get_ip_address_from_file("microsoft.com") << endl;
    
            // Print all DNS entries
            cout << "\nPrinting all DNS entries in EnhancedDistributedDNSManager..." << endl;
            enhancedDnsManager.print_all_entries();
    
            // Print distribution statistics
            cout << "\nPrinting distribution statistics in EnhancedDistributedDNSManager..." << endl;
            enhancedDnsManager.print_distribution_stats();
    
            cout << "\n=== LRU Cache Manager ===" << endl;
            CacheManager lruCache(3);
    
            // Add items to LRU cache
            cout << "\nAdding items to LRU cache..." << endl;
            lruCache.add_update_cache("example.com", "1.1.1.1");
            lruCache.add_update_cache("google.com", "8.8.8.8");
            lruCache.add_update_cache("openai.com", "2.2.2.2");
            lruCache.print_cache();
    
            // Access an item to make it most recently used
            cout << "\nAccessing example.com to make it most recently used..." << endl;
            lruCache.get_ip_address("example.com");
    
            // Add another item to trigger eviction
            cout << "\nAdding github.com to LRU cache (should evict least recently used: google.com)..." << endl;
            lruCache.add_update_cache("github.com", "3.3.3.3");
            lruCache.print_cache();
    
            cout << "\n=== LFU Cache Manager ===" << endl;
            LFUCacheManager lfuCache(3);
    
            // Add items to LFU cache
            cout << "\nAdding items to LFU cache..." << endl;
            lfuCache.add_update_cache("example.com", "1.1.1.1");
            lfuCache.add_update_cache("google.com", "8.8.8.8");
            lfuCache.add_update_cache("openai.com", "2.2.2.2");
            lfuCache.print_cache();
    
            // Access items to increase frequency
            cout << "\nAccessing example.com twice to increase its frequency..." << endl;
            lfuCache.get_ip_address("example.com");
            lfuCache.get_ip_address("example.com");
    
            cout << "Accessing google.com once to increase its frequency..." << endl;
            lfuCache.get_ip_address("google.com");
    
            // Add another item to trigger eviction
            cout << "\nAdding github.com to LFU cache (should evict least frequently used: openai.com)..." << endl;
            lfuCache.add_update_cache("github.com", "3.3.3.3");
            lfuCache.print_cache();
    
            cout << "\n=== LIFO Cache Manager ===" << endl;
            LIFOCacheManager lifoCache(3);
    
            // Add items to LIFO cache
            cout << "\nAdding items to LIFO cache..." << endl;
            lifoCache.add_update_cache("example.com", "1.1.1.1");
            lifoCache.add_update_cache("google.com", "8.8.8.8");
            lifoCache.add_update_cache("openai.com", "2.2.2.2");
            lifoCache.print_cache();
    
            // Add another item to trigger eviction
            cout << "\nAdding github.com to LIFO cache (should evict last added: openai.com)..." << endl;
            lifoCache.add_update_cache("github.com", "3.3.3.3");
            lifoCache.print_cache();
    
        } catch (const exception& e) {
            cout << "Unhandled error: " << e.what() << endl;
        }
    
        return 0;
    }
