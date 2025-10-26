#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <queue>
#include <tuple>
#include <algorithm>
#include <cctype>
using namespace std;
namespace fs = filesystem;
const string INPUT = "input";
const string OUTPUT = "output";
const string NOTHING = "nothing";

struct Prod{
    string name;
    vector <vector<string>> trans;
    string amount_rem;
    string rial_rem;
    string total_prof;
}; 


// Trim from the start (in place)
static inline void ltrim(string &s) 
{
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) 
    {
        return !isspace(ch);
    }));
}

// Trim from the end (in place)
static inline void rtrim(string &s) 
{
    s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) 
    {
        return !isspace(ch);
    }).base(), s.end());
}

// Trim from both ends (in place)
static inline void trim(string &s) 
{
    ltrim(s);
    rtrim(s);
}

void log_message(const string& source, const string& level, const string& message)
{
    cout << "[" << level << " " << source << "] " << message << endl;
}

void read_warehouse_csv_file(vector<Prod>& ware_info, string dir, string file_name)
{
    string file_path = dir + "/" + file_name + ".csv";
    fstream file(file_path);
    if(!file.is_open())
        log_message(file_name, "error", "couldn't open " + file_name + ".csv");
    string line;
    while (getline(file, line)) 
    {  
        vector<string> words;
        string word; 
        stringstream ss(line); 
        while (getline(ss, word, ',')) 
        {
            trim(word); // Remove leading/trailing spaces
            words.push_back(word);
        } 

        int index = -1;
        for(int i = 0; i < ware_info.size(); i++)
        {
            if(words[0] == ware_info[i].name)
            {
                index = i;
            }
        }

        if(index == -1)
        {
            Prod new_prod;
            new_prod.name = words[0];
            new_prod.trans.push_back(words);
            ware_info.push_back(new_prod);
        }
        else
        {
            ware_info[index].trans.push_back(words);
        }
    }
    file.close();
}

void calculate_remaining_amount_and_profit(const vector<vector<string>>& transactions, vector<Prod>& ware_info, int index) 
{
    queue<pair<int, int>> inputQueue; // {price_per_unit, amount}
    float totalProfit = 0;              // Total profit

    for (const auto& transaction : transactions) 
    {
        string name = transaction[0];
        float price_per_unit = stof(transaction[1]);
        int amount = stof(transaction[2]);
        string type = transaction[3];

        if (type == "input") 
        {
            // Add input to the queue
            inputQueue.push({price_per_unit, amount});
        } 
        else if (type == "output") 
        {
            float remainingOutput = amount; // Output amount to process

            while (remainingOutput > 0 && !inputQueue.empty()) 
            {
                auto [inputPrice, inputAmount] = inputQueue.front();
                inputQueue.pop();

                if (inputAmount >= remainingOutput) 
                {
                    // Calculate profit for the portion used
                    totalProfit += remainingOutput * float(price_per_unit - inputPrice);

                    // Update the input batch if there's leftover
                    if (inputAmount > remainingOutput) 
                    {
                        inputQueue.push({inputPrice, inputAmount - remainingOutput});
                    }

                    remainingOutput = 0; // All output processed
                } else {
                    // Consume entire input batch
                    totalProfit += inputAmount * float(price_per_unit - inputPrice);
                    remainingOutput -= inputAmount; // Reduce the remaining output by the amount consumed
                }
            }
        }
    }
    float remainingAmount = 0;
    float remainingPrice = 0;

    while (!inputQueue.empty()) 
    {
        auto [price, amount] = inputQueue.front();
        inputQueue.pop();
        remainingAmount += amount;
        remainingPrice += float(price * amount);
    }

    // Update ware_info with the results
    ware_info[index].amount_rem = to_string(remainingAmount);
    ware_info[index].rial_rem = to_string(remainingPrice);
    ware_info[index].total_prof = to_string(totalProfit); 
}


void calculate_remaining_amount_1(vector<Prod>& ware_info)
{
    for(int i = 0; i < ware_info.size(); i++)
    {
        calculate_remaining_amount_and_profit(ware_info[i].trans, ware_info, i);
    }
}

void split_all_products_name(string input, vector<string>& wanted_products) 
{
    auto split_by_comma = [](const string& str, vector<string>& result) {
        stringstream ss(str);
        string item;
        while (getline(ss, item, ',')) 
        {
            item.erase(item.begin(), find_if(item.begin(), item.end(), [](unsigned char ch) { return !isspace(ch); }));
            item.erase(find_if(item.rbegin(), item.rend(), [](unsigned char ch) { return !isspace(ch); }).base(), item.end());
            result.push_back(item);
        }
    };

    split_by_comma(input, wanted_products);
}

void communicate_with_products(vector<Prod> ware_info, const string ware_name, vector<string> all_products)
{
    for(int i = 0; i < all_products.size(); i++)
    {
        int is = 0;
        int index = -1;
        for(int j = 0; j < ware_info.size(); j++)
        {
            if(ware_info[j].name == all_products[i])
            {
                is = 1;
                index = j;
            }
        }
        
         // Create named pipe for each product
        string fifo_name = ware_name + "_" + all_products[i];
        if (mkfifo(fifo_name.c_str(), 0666) == -1 && errno != EEXIST) 
        {
            perror("[error warehouse] Failed to create FIFO");
            continue;
        }
        log_message(ware_name, "info", "Created FIFO " + fifo_name);

        // Open FIFO for writing
        int fifo_fd = open(fifo_name.c_str(), O_WRONLY);
        if (fifo_fd == -1) 
        {
            perror("[error warehouse] Failed to open FIFO for writing");
            continue;
        }

        if(is)
        {
            // Prepare information for the product
            string data = ware_info[index].amount_rem + "/" + ware_info[index].rial_rem;
            if (write(fifo_fd, data.c_str(), data.size()) == -1) 
            {
                perror("[error warehouse] Failed to write to FIFO");
            } 
            else 
            {
                log_message(ware_name, "info", "Sent data to product " + all_products[i]);
            }

            close(fifo_fd); // Close FIFO after writing
            log_message(ware_name, "info", "Close named pipe " + fifo_name);
        }
        else if(is == 0)
        {
            // Prepare information for the product
            string data = NOTHING;
            if (write(fifo_fd, data.c_str(), data.size()) == -1) 
            {
                perror("[error warehouse] Failed to write to FIFO");
            } 
            else 
            {
                log_message(ware_name, "info", "Sent data to product " + all_products[i]);
            }

            close(fifo_fd); // Close FIFO after writing
            log_message(ware_name, "info", "Close named pipe " + fifo_name);
        }

    }
}

string sum_partial_profits(vector<Prod> prods, vector<string> wanted_prods)
{
    int total_prof = 0;
    for(int i = 0; i < prods.size(); i++)
    {
        for(int j = 0; j < wanted_prods.size(); j++)
        {
            if(prods[i].name == wanted_prods[j])
            {
                total_prof += stoi(prods[i].total_prof);
            }
        }
    }

    return to_string(total_prof);
}

string read_product_list_from_the_parent(int read_fd, string ware_name)
{
    char buffer[256];
    ssize_t bytes_read = read(read_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) 
    {
        perror("[error product] Failed to read from parent");
        exit(1);
    }
    buffer[bytes_read] = '\0';
    log_message(ware_name, "info", "Received products list from the parent");

    return string(buffer);
}

int main(int argc, char* argv[]) 
{
    if (argc != 5) 
    {
        log_message("warehouse", "error", "Invalid arguments.");
        exit(1);
    }

    string ware_name = argv[1];
    int read_fd = atoi(argv[2]);  
    int write_fd = atoi(argv[3]);
    string dir_path = argv[4];

    // Read product list from the parent and process them
    string P;
    P = read_product_list_from_the_parent(read_fd, ware_name);
    vector<string> wanted_products;
    split_all_products_name(P, wanted_products);

    //Calculation
    vector<Prod> ware_info; 
    read_warehouse_csv_file(ware_info, dir_path, ware_name);
    calculate_remaining_amount_1(ware_info);

    
    // Send information to each product process via named pipes
    communicate_with_products(ware_info, ware_name, wanted_products);

    // Perform calculations and send the result back to the parent
    string result = sum_partial_profits(ware_info, wanted_products);
    write(write_fd, result.c_str(), result.size());
    log_message(ware_name, "info", "Sent final result to the parent");

    close(read_fd);
    close(write_fd);


    return 0;
}

