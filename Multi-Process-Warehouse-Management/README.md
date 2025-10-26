# Multi-Process Warehouse Management System

## Overview

A distributed inventory management system implemented in C++ that uses multi-processing and inter-process communication (IPC) mechanisms to manage warehouse inventory and calculate profits across multiple warehouses. The system demonstrates advanced operating system concepts including process forking, pipe communication, and named pipes (FIFOs).

## Features

- **Multi-Process Architecture**: Spawns separate child processes for warehouses and products
- **Inter-Process Communication**: Uses anonymous pipes and named pipes (FIFOs) for process synchronization
- **FIFO Inventory Accounting**: Implements First-In-First-Out (FIFO) method for inventory cost calculation
- **Profit Calculation**: Calculates total profit across all warehouses based on transactions
- **Dynamic Process Management**: Efficiently manages multiple concurrent processes
- **CSV Data Integration**: Reads transaction data from CSV files

## Architecture

The system consists of three main components:

1. **Main Process** (`main.cpp`): Orchestrates the entire system by:
   - Reading product catalog from `Parts.csv`
   - Discovering warehouse CSV files
   - Taking user input for desired products
   - Forking child processes for warehouses and products
   - Managing IPC via pipes and FIFOs
   - Aggregating and displaying results

2. **Warehouse Processes** (`warehouse.cpp`): Each warehouse process:
   - Reads its own transaction CSV file
   - Processes transactions using FIFO accounting method
   - Calculates remaining inventory and profit
   - Communicates with product processes via named pipes
   - Returns profit calculations to the parent process

3. **Product Processes** (`product.cpp`): Each product process:
   - Receives data from all warehouse processes
   - Aggregates remaining inventory quantities and prices
   - Returns consolidated product information to the parent

## Process Communication Flow

```
Main Process
    ├──> Warehouse Process 1 ──FIFO──> Product Process 1
    ├──> Warehouse Process 2 ──FIFO──> Product Process 1
    └──> Warehouse Process N ──FIFO──> Product Process M
```

## Build Instructions

### Prerequisites
- C++17 compatible compiler (GCC 7.0+ or Clang 5.0+)
- Make build system
- Unix-like operating system (Linux, macOS, WSL)

### Compilation

```bash
# Build all executables
make

# Alternatively, build individual components
make main
make product
make warehouse

# Clean build artifacts
make clean
```

## Usage

1. **Prepare your data directory**:
   - Ensure `stores/` directory exists
   - Add a `Parts.csv` file listing all products
   - Add warehouse CSV files (e.g., `Tehran.csv`, `Mashhad.csv`)

2. **Run the program**:
   ```bash
   ./main stores/
   ```

3. **Input selection**:
   - The program will display all available products
   - Enter the names of products you want to query (space-separated)

4. **View results**:
   - Total remaining quantity for each requested product
   - Total remaining value for each requested product
   - Total profit across all stores

## Data Format

### Parts.csv
A comma-separated list of all products available in the system:
```
Product1,Product2,Product3,...
```

### Warehouse CSV Files
Each warehouse file contains transaction records with the following format:
```
ProductName,PricePerUnit,Amount,Type
```

Where:
- `ProductName`: Name of the product
- `PricePerUnit`: Price per unit
- `Amount`: Quantity
- `Type`: Either "input" (stocking) or "output" (selling)

## Example

```bash
$ ./main stores/
Hi
We have these products:
1-  Apple
2-  Orange
3-  Banana
Enter the names of the parts you want, seperated by a space: Apple Orange

Apple
    Total Leftover Quantity = 150
    Total Leftover Price = 7500
Orange
    Total Leftover Quantity = 200
    Total Leftover Price = 8000
Total profit of all stores: 5000
```

## Technical Implementation Details

### Process Management
- Uses `fork()` to create child processes
- Uses `waitpid()` for proper process synchronization
- Implements pipe communication for parent-child data exchange

### IPC Mechanisms
- **Anonymous Pipes**: For bidirectional communication between parent and child processes
- **Named Pipes (FIFOs)**: For peer-to-peer communication between warehouse and product processes
- Uses `mkfifo()` to create named pipes dynamically

### Algorithm
- **FIFO Accounting**: When processing output transactions, the system consumes the oldest input inventory first
- Profit is calculated as: `(Sell Price - Buy Price) × Quantity`
- Handles partial consumption of inventory batches

### Error Handling
- Comprehensive error checking for file operations
- Process creation and communication error handling
- Graceful cleanup of resources

## Learning Outcomes

This project demonstrates proficiency in:
- Operating system concepts (processes, IPC)
- System programming in C++
- Multi-process application design
- Synchronization and inter-process communication
- File I/O and data parsing
- Build systems and Makefiles

## License

This project is created for educational purposes as part of an Operating Systems course assignment.
