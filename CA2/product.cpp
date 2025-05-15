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
#include <regex>
using namespace std;
namespace fs = filesystem;
const string NOTHING = "nothing";

void log_message(const string& source, const string& level, const string& message)
{
    cout << "[" << level << " " << source << "] " << message << endl;
}

void process_names_message(string input, float& rem_amount, float& rem_rial, string product_name) 
{
    if (input == NOTHING) 
    {
        rem_amount += 0;
        rem_rial += 0;
    }
    else 
    {
        int pos = 0;
        pos = input.find('/');
        input.substr(0, pos);
        float number1 = stof(input.substr(0, pos));
        float number2 = stof(input.substr(pos + 1, input.size()));
        rem_amount += number1;
        rem_rial += number2;
    }
}

void receive_from_warehouses(const string product_name, vector<string> warehouses, float& rem_amount, float& rem_rial)
{
    for (string warehouse : warehouses)
    {
        // Open the FIFO for reading
        string fifo_name = warehouse + "_" + product_name;
        int fifo_fd = -1;
        while(true) 
        {
            fifo_fd = open(fifo_name.c_str(), O_RDONLY);
            if (fifo_fd != -1) 
            {
                log_message(product_name, "info", "Successfully opened FIFO: " + fifo_name);
                break;
            }
            log_message(product_name, "warning",fifo_name + " Waiting...");
            sleep(1); // Retry after 1 second
        }
        if (fifo_fd == -1) 
        {
            log_message(product_name, "error", "Unable to open FIFO after multiple attempts.");
            exit(1);
        }

        log_message(product_name, "info", "Opened FIFO " + fifo_name + " for reading");

        // Read data from the FIFO
        char buffer[256];
        ssize_t bytes_read;
        while ((bytes_read = read(fifo_fd, buffer, sizeof(buffer) - 1)) > 0) 
        {
            buffer[bytes_read] = '\0';
            log_message(product_name, "info", "Received from " + warehouse + ": " + string(buffer));
            process_names_message(string(buffer), rem_amount, rem_rial, product_name);
        }

        if (bytes_read == -1) 
        {
            perror(("[error product] Failed to read from FIFO " + fifo_name).c_str());
        }

        close(fifo_fd); // Close FIFO after reading
        if (unlink(fifo_name.c_str()) == -1) 
        {
            perror(("[error warehouse] Failed to delete FIFO " + fifo_name).c_str());
        }
        log_message(product_name, "info", "Deleted named pipe");
    }
}

void read_warehouse_list_from_the_parent(vector<string>& warehouses, int read_fd, string product_name)
{
    char buffer[256];
    ssize_t bytes_read = read(read_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) 
    {
        perror("[error product] Failed to read from parent");
        exit(1);
    }
    buffer[bytes_read] = '\0';
    log_message(product_name, "info", "Received warehouses list from the parent");

    string warehouses_arg(buffer);
    size_t pos = 0;
    while ((pos = warehouses_arg.find(',')) != string::npos) 
    {
        warehouses.push_back(warehouses_arg.substr(0, pos));
        warehouses_arg.erase(0, pos + 1);
    }
    warehouses.push_back(warehouses_arg);
}

int main(int argc, char* argv[]) 
{
    if (argc != 4) 
    { // Expecting 4 arguments: product name, read FD, write FD
        log_message("product", "error", "Invalid arguments.");
        exit(1);
    }

    string product_name = argv[1];
    int read_fd = atoi(argv[2]); 
    int write_fd = atoi(argv[3]); 

    // Read warehouse list from the parent
    vector<string> warehouses;
    read_warehouse_list_from_the_parent(warehouses, read_fd, product_name);

    // Communicate with each warehouse using named pipes
    float rem_amount = 0;
    float rem_rial = 0;
    receive_from_warehouses(product_name, warehouses, rem_amount, rem_rial);

    // Perform calculations and send the result back to the parent
    string result = to_string(rem_amount) + "/" + to_string(rem_rial);
    write(write_fd, result.c_str(), result.size());
    log_message(product_name, "info", "Sent final result to the parent");

    close(read_fd);
    close(write_fd);

    return 0;
}


    