#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;
namespace fs = filesystem;


string PARTS = "Parts.csv";
string PROD_PROG = "./product";
string WARE_PROG = "./warehouse";

struct Product{
    string p_name;
    int child_to_par = -1;
    float rem_amount = 0;
    float rem_rial = 0;
};
struct Warehouse{
    string w_name;
    int child_to_par = -1;
    float total_profit = 0;
};

void log_message(const string& source, const string& level, const string& message) 
{
    cout << "[" << level << " " << source << "] " << message << endl;
}

vector<Product> make_products_objects(vector<string> prod)
{
    vector<Product> products;
    for(int i = 0; i < prod.size(); i++)
    {
        Product new_prod;
        new_prod.p_name = prod[i];
        products.push_back(new_prod); 
    }
    return products;
};

vector<Product> read_parts_file(string dir_path, string& P)
{
    string parts_path = dir_path + "/" + PARTS;
    fstream file(parts_path);
    if(!file.is_open())
        cerr << "[error main] Could not open parts.csv!" << endl;
    log_message("main", "info", "Openning " + PARTS + " was successfull.");
    string line;
    vector<string> products;
    while(getline(file, line))
    {
        stringstream ss(line);
        string prod;
        P = line;
        while(getline(ss, prod, ','))
        {
            products.push_back(prod);
        } 
    }
    
    file.close();
    log_message("main", "info", "Closing " + PARTS + " was successfull.");
    vector<Product> prods = make_products_objects(products);
    return prods;
};

vector<Warehouse> make_warehouses_objects(vector<string> wh)
{
    vector<Warehouse> warehouses;
    for(int i = 0; i < wh.size(); i++)
    {
        Warehouse new_wh;
        new_wh.w_name = wh[i];
        warehouses.push_back(new_wh); 
    }
    return warehouses;
};
vector<Warehouse> get_csv_files(string dir_path) 
{
    vector<string> csv_files;

    for (const auto& entry : fs::directory_iterator(dir_path)) 
    {
        if (entry.is_regular_file()) 
        {
            string file_name = entry.path().filename().string();

            if (file_name != PARTS && file_name.size() > 4 &&
                file_name.substr(file_name.size() - 4) == ".csv") 
            {
                size_t pos = file_name.find_last_of('.');
                string main_name = file_name.substr(0, pos);
                csv_files.push_back(main_name);
            }
        }
    }

    vector<Warehouse> whs = make_warehouses_objects(csv_files);
    return whs;
}

void process_product_message(vector<Product>& products, string message, int index)
{
    size_t delimiter_pos = message.find('/');
    if (delimiter_pos == string::npos) 
    {
        log_message("main", "error", "Invlid recieved message format");
    }

    string first_part = message.substr(0, delimiter_pos);
    string second_part = message.substr(delimiter_pos + 1);

    products[index].rem_amount = stof(first_part);
    products[index].rem_rial = stof(second_part);
}

void process_warehouse_message(vector<Warehouse>& warehouses, string message, int index)
{
    warehouses[index].total_profit = stof(message);
}

void wait_for_childs(vector<pid_t>& warehouse_pids, vector<pid_t>& product_pids)
{
    for (pid_t pid : warehouse_pids) {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("[error main] Failed to wait for warehouse process");
        } else {
            log_message("main", "info", "Warehouse process " + to_string(pid) + " exited.");
        }
    }
    for (pid_t pid : product_pids) {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("[error main] Failed to wait for product process");
        } else {
            log_message("main", "info", "Product process " + to_string(pid) + " exited.");
        }
    }
}

void fork_and_exec(vector<Product>& products, vector<Warehouse>& warehouses, const string dir_path, string P, string WP, vector<string> wanted_products) 
{
    vector<pid_t> warehouse_pids;
    vector<pid_t> product_pids;

    for (size_t i = 0; i < warehouses.size(); ++i) 
    {
        int parent_to_child[2]; 
        int child_to_parent[2]; 
        if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1) 
        {
            perror("[error main] Failed to create pipes for warehouse");
            exit(1);
        }
        log_message("main", "info", "Created pipes for warehouse " + warehouses[i].w_name);

        pid_t pid = fork();
        if (pid == 0) 
        { 
            close(parent_to_child[1]); 
            close(child_to_parent[0]); 

            string read_pipe_fd = to_string(parent_to_child[0]);
            string write_pipe_fd = to_string(child_to_parent[1]);

            execl(WARE_PROG.c_str(), WARE_PROG.c_str(), warehouses[i].w_name.c_str(),
                  read_pipe_fd.c_str(), write_pipe_fd.c_str(), dir_path.c_str(), nullptr);
            perror("[error product] Failed to exec warehouse process");
            exit(1);
        } 
        else if (pid < 0) 
        {
            perror("[error main] Failed to fork warehouse process");
            exit(1);
        }
        warehouse_pids.push_back(pid);
        log_message("main", "info", "Forked warehouse process for " + warehouses[i].w_name);
        
        close(parent_to_child[0]);
        close(child_to_parent[1]);

        string prod_plus_wanted = WP;
        write(parent_to_child[1], prod_plus_wanted.c_str(), prod_plus_wanted.size() + 1);
        log_message("main", "info", "Send list to " + warehouses[i].w_name);
        
        warehouses[i].child_to_par = child_to_parent[0];
        close(parent_to_child[1]);
    }

    for (size_t i = 0; i < products.size(); ++i) 
    {
        for(int j = 0; j < wanted_products.size(); j++)
        {
            if(products[i].p_name == wanted_products[j])
            {    int parent_to_child[2]; 
                int child_to_parent[2]; 
                if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1) 
                {
                    perror("[error main] Failed to create pipes for product");
                    exit(1);
                }
                log_message("main", "info", "Created pipes for product " + products[i].p_name);

                pid_t pid = fork();
                if (pid == 0) 
                {  
                    close(parent_to_child[1]); 
                    close(child_to_parent[0]);


                    string read_pipe_fd = to_string(parent_to_child[0]);
                    string write_pipe_fd = to_string(child_to_parent[1]);

                    execl(PROD_PROG.c_str(), PROD_PROG.c_str(), products[i].p_name.c_str(),
                        read_pipe_fd.c_str(), write_pipe_fd.c_str(), nullptr);
                    perror("[error product] Failed to exec product process");
                    exit(1);
                } 
                else if (pid < 0) 
                {
                    perror("[error main] Failed to fork product process");
                    exit(1);
                }
                product_pids.push_back(pid);
                log_message("main", "info", "Forked product process for " + products[i].p_name);
                close(parent_to_child[0]);
                close(child_to_parent[1]);
                // Write warehouse list to the child via the parent-to-child pipe
                string warehouses_arg;
                for (size_t j = 0; j < warehouses.size(); ++j) 
                {
                    warehouses_arg += warehouses[j].w_name;
                    if (j < warehouses.size() - 1) warehouses_arg += ",";
                }
                write(parent_to_child[1], warehouses_arg.c_str(), warehouses_arg.size() + 1);
                log_message("main", "info", "Send list to " + products[i].p_name);
                
                products[i].child_to_par = child_to_parent[0];
                close(parent_to_child[1]);
            }
        }
    }


    // Read result from the child via the child-to-parent pipe
    for(int i = 0; i < products.size(); i++)
    {
        for(int j = 0; j < wanted_products.size(); j++)
        {
            if(products[i].p_name == wanted_products[j])
            {
                char buffer[256];
                ssize_t bytes_read = read(products[i].child_to_par, buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) 
                {
                    buffer[bytes_read] = '\0';
                    log_message("main", "info", "Received from product " + products[i].p_name);
                    process_product_message(products, string(buffer), i);
                } 
                else if (bytes_read == -1) 
                {
                    perror("[error main] Failed to read from child-to-parent pipe");
                }
                close(products[i].child_to_par);
            }
        }
    }
    // Read result from the child via the child-to-parent pipe
    for(int i = 0; i < warehouses.size(); i++)
    {
        char buffer[256];
        ssize_t bytes_read = read(warehouses[i].child_to_par, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) 
        {
            buffer[bytes_read] = '\0';
            log_message("main", "info", "Received from warehouse " + warehouses[i].w_name);
            process_warehouse_message(warehouses, string(buffer), i);
        } 
        else if (bytes_read == -1) 
        {
            perror("[error main] Failed to read from child-to-parent pipe");
        }
        close(warehouses[i].child_to_par);
    }

    wait_for_childs(warehouse_pids, product_pids);
}

vector<string> get_input_from_user(vector<Product> prods)
{
    cout << "Hi\nWe have these products:\n";
    for(int i = 0; i < prods.size(); i++)
    {
        cout << i + 1 << "-  " << prods[i].p_name << endl;
    }
    cout << "Enter the names of the parts you want, seperated by a space: ";
    string prompt;
    getline(cin, prompt);
    vector<string> wanted_prods;
    stringstream ss(prompt); 
    string word;

    while (ss >> word) 
    {
        wanted_prods.push_back(word);
    }
    
    return wanted_prods;
}

void show_result(vector<Product> prods, vector<Warehouse> warehouses, vector<string> wanted_prods)
{
    for(int i = 0; i < wanted_prods.size(); i++)
    {
        for(int j = 0; j < prods.size(); j++)
        {
            if(wanted_prods[i] == prods[j].p_name)
            {
                cout << prods[j].p_name << endl;
                cout << "\tTotal Leftover Quantity = " << to_string(int(prods[j].rem_amount)) << endl;
                cout << "\tTotal Leftover Price = " << to_string(int(prods[j].rem_rial)) << endl;
            }   
        }
    }

    int sum_total_prof = 0;
    for(int n = 0; n < warehouses.size(); n++)
    {
        sum_total_prof += warehouses[n].total_profit;
    }
    cout << "Total profit of all stores: " << sum_total_prof << endl;
}

string convert_wanted_to_a_string(vector<string> wanted_products)
{
    string flatten_wanted;
    for (size_t i = 0; i < wanted_products.size(); ++i)
     {
        flatten_wanted += wanted_products[i];
        if (i != wanted_products.size() - 1) 
        {
            flatten_wanted += ",";
        }
    }
    return flatten_wanted;
}

int main(int argc, char* argv[])
{
    if(argc != 2)
        perror("Invalid Arguments");

    string dir_path = argv[1];
    string P;
    string WP;
    vector<Product> products = read_parts_file(dir_path, P);
    vector<Warehouse> warehoses = get_csv_files(dir_path);
    vector<string> wanted_prod = get_input_from_user(products);
    WP = convert_wanted_to_a_string(wanted_prod);

    fork_and_exec(products, warehoses, dir_path, P, WP, wanted_prod);
    show_result(products, warehoses, wanted_prod);
}
