#ifndef IR_NODE_H
#define IR_NODE_H

#include <stdint.h>
#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <memory>

using namespace std;

void panic(const char *fmt, ...);
void assert(bool condition, const char *fmt, ...);

static const char *opname[] = {"Const", "NoOp",
                               "Var", "Plus", "Minus", "Times", "Divide", "Power",
                               "Sin", "Cos", "Tan", "ASin", "ACos", "ATan", "ATan2", 
                               "Abs", "Floor", "Ceil", "Round",
                               "Exp", "Log", "Mod", 
                               "LT", "GT", "LTE", "GTE", "EQ", "NEQ",
                               "And", "Or", "Nand", "Load",
                               "IntToFloat", "FloatToInt", 
                               "LoadImm", "PlusImm", "TimesImm"};


enum OpCode {Const = 0, NoOp, 
             Var, Plus, Minus, Times, Divide, Power,
             Sin, Cos, Tan, ASin, ACos, ATan, ATan2, 
             Abs, Floor, Ceil, Round,
             Exp, Log, Mod, 
             LT, GT, LTE, GTE, EQ, NEQ,
             And, Or, Nand,
             Load,
             IntToFloat, FloatToInt, 
             LoadImm, PlusImm, TimesImm};

// One node in the intermediate representation
class IRNode {
public:

    typedef shared_ptr<IRNode> Ptr;
    typedef weak_ptr<IRNode> WeakPtr;

    enum Type {Unknown = 0, Float, Bool, Int};   

    // Opcode
    OpCode op;
        
    // Any immediate data. Mostly useful for the Const op.
    float fval;
    int ival;

    // Vector width. Should be one or four on X64.
    //
    // TODO: this is currently always set to 1 even though we're
    // forcibly vectorizing across X
    int width;

    // Inputs - whose values do I depend on?
    vector<Ptr> inputs;    
        
    // Who uses my value?
    vector<WeakPtr> outputs;

            
    // Does this op depend on any vars or memory?
    bool constant;

    // What register will this node be computed in? -1 indicates no
    // register has been allocated. 0-15 indicates a GPR, 16-31
    // indicates an SSE register. It will be -1 until register
    // allocation takes place.
    signed char reg;
               
    // What level of the for loop will this node be computed at?
    // Right now 0 is outermost, representing consts, and 4 is
    // deepest, representing iteration over channels.
    signed char level;

    // What is the type of this expression?
    Type type;

    // A tag used by recursive algorithms that need to add marks to different nodes
    int tag;

    // Destructor. Don't call delete - use Ptrs and WeakPtrs instead.
    ~IRNode();

    // Make a float constant
    static Ptr make(float v);

    // Make an int constant 
    static Ptr make(int v);

    // Make an IRNode with the given opcode and the given inputs and constant values
    static Ptr make(OpCode opcode, 
                    Ptr input1 = NULL, 
                    Ptr input2 = NULL, 
                    Ptr input3 = NULL,
                    Ptr input4 = NULL,
                    int ival = 0,
                    float fval = 0.0f);


    // Return an optimized version of this node. Most optimizations
    // are done by make, but there may be some that can only
    // effectively run after the entire DAG is generated. They go
    // here.
    Ptr optimize();

    // Return a new version of this node with one IRNode replaced with
    // another. Rebuilds and reoptimizes the graph.
    Ptr substitute(Ptr oldNode, Ptr newNode);

    // Assign a loop level to a var. The outputs will be recursively updated too.
    void assignLevel(int);

    // Cast an IRNode to a different type
    Ptr as(Type t);

    // Recursively print out the complete expression this IRNode
    // computes (e.g. x+y*17). This can get long.
    void printExp();

    // Print out which operation occurs on what registers (e.g. xmm0 =
    // xmm1 + xmm2). Must be called after registers are assigned.
    void print();

    // Save out a .dot file showing all nodes in existence and how they connect
    static void saveDot(const char *filename);

    // Make another copy of the sole shared pointer to this object
    Ptr ptr() {return self.lock();}

    // All nodes in existence
    static vector<WeakPtr> allNodes;

protected:
    // All the const float nodes
    static map<float, WeakPtr> floatInstances;

    // All the int nodes
    static map<int, WeakPtr> intInstances;

    // The correct way for IRNode methods to create new nodes.
    static Ptr makeNew(float);
    static Ptr makeNew(int);
    static Ptr makeNew(Type, OpCode, const vector<Ptr> &input, int, float);

    // The actual constructor. Only used by the makeNew methods, which
    // are only used by the make methods.
    IRNode(Type t, OpCode opcode, 
           const vector<Ptr> &input,
           int iv, float fv);
    
    // A slightly more generate make function that takes a vector of children for inputs
    static Ptr IRNode::make(OpCode opcode,
                            vector<Ptr> inputs,
                            int ival = 0, float fval = 0.0f);
    


    // A weak reference to myself. 
    WeakPtr self;

    // Reorder summations from low to high level. (x+(y+1)) is cheaper
    // to compute than ((x+y)+1) because more can be moved into an
    // outer loop. This is done by the optimize call and also by make.
    Ptr rebalanceSum();
    void collectSum(vector<pair<Ptr, bool> > &terms, bool positive = true);

};


#endif






