# Tomasulo Algorithm Simulator with Register Renaming

This project implements a cycle-accurate Tomasulo algorithm simulator in C, extended with **true register renaming** using an additional pool of physical registers.  
The simulator models issue, execute, and write-back stages, reservation stations, CDB broadcasting, functional unit availability, and renaming–based hazard elimination.

This project demonstrates core computer architecture concepts such as out-of-order execution, register aliasing, hazard avoidance, and dynamic scheduling.

---

## 🚀 Key Features

### ✔ Full Tomasulo Pipeline
- Reservation Stations for ADD, MUL, LOAD, and STORE
- Common Data Bus (CDB) broadcasting
- Functional units with real latencies
- Operand readiness tracking via Qi tags
- RAW hazard handling

### ✔ Register Renaming (Major Contribution)
- 16 architectural + 16 rename physical registers  
- Logical-to-physical register mapping table  
- Physical register busy table + ownership tracking  
- Free-list allocation mechanism  
- Safe recycling of stale physical registers  
- Eliminates **WAW** and **WAR** hazards  
- Enables higher instruction-level parallelism

### ✔ Detailed Debug Output
Each simulation cycle prints:
- Reservation Station (RS) state
- Logical register file (after mapping)
- Physical register rename pool (p16–p31)
- Qi tags and value propagation

This makes it easy to visualize pipeline behavior.

---

## 📁 File Structure
├── tomasulo.c # Main simulation loop (issue/execute/write back)
├── arch.c # Register file, renaming logic, RS maintenance
├── arch.h # Data structures, FU/RS definitions, renaming APIs
├── inst.c # Instruction initialization + printing
├── inst.h # Instruction struct + op definitions
├── Makefile # Build script
└── Register Renaming Implementation Report.docx # Full analysis report


