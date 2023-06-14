#pragma once

#include "Parser.h"

MakeStrong(FuncID, i32);
MakeStrong(TypeID, i32);

struct Runner
{
    struct Func
    {
        string name;
        bool builtin{false};
    };
    map<string, Func> funcs{{"print",{"print",true}}};

    struct Var
    {
        string type;
        string name;


        string data_string;
        i64 data_i64{};
        f64 data_f64{};

        string ToString()
        {
            if (type == "@str")
                return data_string;
            else if (type == "@i64")
                return ::ToString(data_i64);
            else if (type == "@f64")
                return ::ToString(data_f64);
            return "WTF?";
        }

        template<typename T>
        static Var Create(T& input)
        {
            if constexpr (std::same_as<T,i64>)
                return Var{.type="@i64", .data_i64=input};
            else if constexpr (std::same_as<T,f64>)
            {
                return Var{.type="@f64", .data_f64=input};
            }
            else if constexpr (std::same_as<T,std::string>)
            {
                return Var{.type="@str", .data_string=input};
            }
            else
            {
                cout << "Unsupported type to Var::Create!!" << endl;
                std::abort();
            }
        }
    };

    static constexpr struct InstDef
    {
        enum struct TYPE
        {
            NOP, //0 is nop
            MOVE, //this is a *REAL* move, the source is left unusable!
            LITERAL_STR, //loads a string literal
            LITERAL_I64, //loads a i64 literal
            LITERAL_F64, //loads a f64 literal
            CALL, //call a function.
            ADDMOVE, //this moves value from id_params[1], adds it to id_params[0]
            SUBMOVE, //this moves value from id_params[1], subtracts it from id_params[0]
            MULMOVE,
            DIVMOVE,
            LOAD, //loads a rvalue
            STORE, //declares a variable. params: type, name, value
            N
        } type{TYPE::NOP};
        const char* name="";
        int creates{-1}, destroys{-1}; //not -1, if it destroys / creates a register

    } instdefs[int(InstDef::TYPE::N)] =
    {
        {InstDef::TYPE::NOP, "NOP", -1,-1},
        {InstDef::TYPE::MOVE, "MOVE", 1,0},
        {InstDef::TYPE::LITERAL_STR, "LITERAL_STR", 0,-1},
        {InstDef::TYPE::LITERAL_I64, "LITERAL_I64", 0,-1},
        {InstDef::TYPE::LITERAL_F64, "LITERAL_F64", 0,-1},
        {InstDef::TYPE::CALL, "CALL",-1,-1},
        {InstDef::TYPE::ADDMOVE, "ADDMOVE", -1,0},
        {InstDef::TYPE::SUBMOVE, "SUBMOVE", -1,0},
        {InstDef::TYPE::MULMOVE, "MULMOVE", -1,0},
        {InstDef::TYPE::DIVMOVE, "DIVMOVE", -1,0},
        {InstDef::TYPE::LOAD, "LOAD", 0,-1},
        {InstDef::TYPE::STORE, "STORE", -1,0},
    };

    struct Instruction
    {
        using TYPE = InstDef::TYPE;
        TYPE type{TYPE::NOP};
        vector<RID> id_params;
        string str_param;
        i64 i64_param{};
        f64 f64_param{};

        void Print()
        {
            cout << instdefs[int(type)].name << ": ";
            for(int i=0; i<id_params.size(); ++i)
                cout << (i>0?",":"") << id_params[i];
            cout << " params, str_param=" << str_param << ", i64_param=" << i64_param << ", f64_param=" << f64_param << endl;
        }
    };

    vector<Instruction> code;
    map<RID,Var> registers;
    map<string,Var> variables;

    void CodeOptimize()
    {
        while(true)
        {
            vector<Instruction> newcode;
            size_t oldsize=code.size();
            for(int i=0; i<code.size();)
            {
                auto& ins1 = code[i];
                auto& ins2 = code[i+1];

                if (i+1 == code.size() || ins1.type != ins2.type || ins1.type != Instruction::TYPE::MOVE || ins1.id_params[1] != ins2.id_params[0])
                {
                    newcode.push_back(code[i]);
                    i+=1;
                    continue;
                }
                //both are now MOVE
                Instruction newins = ins2;
                newins.id_params[0] = ins1.id_params[0]; //give it the src of the former
                newcode.push_back(newins);
                i+=2;
            }
            code = newcode;
            cout << "oldsize " << oldsize << ": " << code.size() << endl;
            if (oldsize==code.size())
                break;
        }
    }

    void CodeRun()
    {
        cout << "Code has " << code.size() << " instructions." << endl;
        for(int c_id=0; c_id<code.size(); ++c_id)
        {
            auto& c = code[c_id];
            /*cout << registers.size() << " registers: " << endl;
            for(auto& [id, var]: registers)
                cout << "  " << id << ": " << var.type << " " << var.name << " = " << var.ToString() << endl;
            cout << variables.size() << " variables: " << endl;
            for(auto& [id, var]: variables)
                cout << "  " << id << ": " << var.type << " " << var.name << " = " << var.ToString() << endl;
            cout << endl;
            cout << "PC:" << c_id << ", ";
            c.Print();*/
            if (c.type == Instruction::TYPE::MOVE)
            {
                RID src = c.id_params[0], dst = c.id_params[1];
                if (!registers.contains(src))
                {
                    cout << "MOVE: Registers do not contain ID " << src << "!" << endl;
                    std::abort();
                }
                registers[dst] = registers[src];
                registers.erase(src);
            }

            else if (c.type == Instruction::TYPE::LITERAL_STR)
            {
                RID dst = c.id_params[0];

                if (registers.contains(dst))
                {
                    cout << "LITERAL_STR: Registers already contain ID " << dst << "!" << endl;
                    std::abort();
                }
                registers[dst] = Var::Create(c.str_param);//{"@str","", c.str_param};
            }
            else if (c.type == Instruction::TYPE::LITERAL_I64)
            {
                RID dst = c.id_params[0];
                if (registers.contains(dst))
                {
                    cout << "LITERAL_I64: Registers already contain ID " << dst << "!" << endl;
                    std::abort();
                }
                registers[dst] = Var::Create(c.i64_param); //{"@i64","", "", c.i64_param}
            }
            else if (c.type == Instruction::TYPE::LITERAL_F64)
            {
                RID dst = c.id_params[0];
                if (registers.contains(dst))
                {
                    cout << "LITERAL_F64: Registers already contain ID " << dst << "!" << endl;
                    std::abort();
                }
                registers[dst] = Var::Create(c.f64_param);
            }
            else if (c.type == Instruction::TYPE::CALL)
            {
                string funcname = registers[c.id_params[0]].data_string;
                if (funcname == "print")
                {
                    for(int i=1; i<c.id_params.size(); ++i)
                        cout << registers[c.id_params[i]].ToString();
                }
                else if (funcname == "println")
                {
                    for(int i=1; i<c.id_params.size(); ++i)
                        cout << registers[c.id_params[i]].ToString();
                    cout << endl;
                }
                else
                {
                    cout << "CALL: Function name " << funcname << " not found." << endl;
                    std::abort();
                }
                for(auto& id: c.id_params)
                    registers.erase(id);
            }
            else if (c.type == Instruction::TYPE::ADDMOVE
                     || c.type == Instruction::TYPE::SUBMOVE
                     || c.type == Instruction::TYPE::MULMOVE
                     || c.type == Instruction::TYPE::DIVMOVE)
            {
                RID src = c.id_params[0], dst = c.id_params[1];
                //TODO: check types here.
                if (registers[src].type != registers[dst].type)
                {
                    cout << "MATHMOVE: src and dst register types not the same" << endl;
                    std::abort();
                }
                //TODO: give each type some traits and query them here instead of this hardcoded if
                if (registers[src].type == "@i64")
                {
                    if (c.type == Instruction::TYPE::ADDMOVE)
                        registers[dst].data_i64 += registers[src].data_i64;
                    if (c.type == Instruction::TYPE::SUBMOVE)
                        registers[dst].data_i64 -= registers[src].data_i64;
                    if (c.type == Instruction::TYPE::MULMOVE)
                        registers[dst].data_i64 *= registers[src].data_i64;
                    if (c.type == Instruction::TYPE::DIVMOVE)
                        registers[dst].data_i64 /= registers[src].data_i64;
                }
                else if (registers[src].type == "@f64")
                {
                    if (c.type == Instruction::TYPE::ADDMOVE)
                        registers[dst].data_f64 += registers[src].data_f64;
                    if (c.type == Instruction::TYPE::SUBMOVE)
                        registers[dst].data_f64 -= registers[src].data_f64;
                    if (c.type == Instruction::TYPE::MULMOVE)
                        registers[dst].data_f64 *= registers[src].data_f64;
                    if (c.type == Instruction::TYPE::DIVMOVE)
                        registers[dst].data_f64 /= registers[src].data_f64;
                }
                else
                {
                    cout << "MATHMOVE: src/dst register type not a number" << endl;
                    std::abort();
                }
                registers.erase(src);
            }
            else if (c.type == Instruction::TYPE::LOAD)
            {
                RID dst = c.id_params[0];
                string varname = c.str_param;
                if (!variables.contains(varname))
                {
                    cout << "LOAD: Variables don't have " << varname << endl;
                    std::abort();
                }
                registers[dst] = variables[varname];
                registers.erase(c.id_params[1]);
            }
            else if (c.type == Instruction::TYPE::STORE)
            {
                auto& type = registers[c.id_params[0]];
                auto& name = registers[c.id_params[1]];
                auto& value = registers[c.id_params[2]];
                string varname = name.data_string;
                if (variables.contains(varname))
                {
                    cout << "STORE: Variables already have " << varname << endl;
                    std::abort();
                }
                Var newvar{.type=type.data_string, .name=name.data_string};
                if (newvar.type == "@str")
                    newvar.data_string = value.data_string;
                else if (newvar.type == "@i64")
                    newvar.data_i64 = value.data_i64;
                else if (newvar.type == "@f64")
                    newvar.data_f64 = value.data_f64;
                else
                {
                    cout << "STORE: Unknown type: " << newvar.type << "." << endl;
                }
                variables[varname] = newvar;
                registers.erase(c.id_params[0]);
                registers.erase(c.id_params[1]);
                registers.erase(c.id_params[2]);
            }
        }
        /*cout << registers.size() << " registers: " << endl;
        for(auto& [id, var]: registers)
            cout << "  " << id << ": " << var.ToString() << endl;
        cout << variables.size() << " variables: " << endl;
        for(auto& [id, var]: variables)
            cout << "  " << id << ": " << var.type << " " << var.name << " = " << var.ToString() << endl;
        cout << endl;*/


    }

    void CodeGen(AST& ast, RID id, int amount_spaces)
    {
        //post-traversal
        AST::Node& node = ast.Get(id);
        for(auto& child: node.children)
            CodeGen(ast, child, amount_spaces+1);

        if (node.data == "PAREN_LEFT" || node.data == "PAREN_RIGHT" || node.data == "STATEMENT_END" || node.data == "COMMA");
        else if (node.data == "LITERAL_STRING")
        {
            code.push_back(Instruction{.type=Instruction::TYPE::LITERAL_STR, .id_params={node.id}, .str_param=node.data2.substr(1,node.data2.size()-2)});
        }
        else if (node.data == "LITERAL_I64")
        {
            code.push_back(Instruction{.type=Instruction::TYPE::LITERAL_I64, .id_params={node.id}, .i64_param=FromString<i64>(node.data2)});
        }
        else if (node.data == "LITERAL_F64")
        {
            code.push_back(Instruction{.type=Instruction::TYPE::LITERAL_F64, .id_params={node.id}, .f64_param=FromString<f64>(node.data2)});
        }
        else if (node.data == "OP_ASSIGN" || node.data == "OP_PLUS" || node.data == "OP_MINUS" || node.data == "OP_MUL" || node.data == "OP_DIV");
        else if (node.data == "#op_term" || node.data == "#op_factor"); //TODO: maybe just propagate the op to these nodes?
        else if (node.data == "NAME_IDENTIFIER" || node.data == "TYPE_IDENTIFIER")
        {
            code.push_back(Instruction{.type=Instruction::TYPE::LITERAL_STR, .id_params={node.id}, .str_param=node.data2});
        }
        else if (node.data == "#literal" && node.children.size() == 1)
        {
            code.push_back(Instruction{.type=Instruction::TYPE::MOVE, .id_params={node.children[0], node.id}});
        }
        else if (node.data == "#primary" && node.children.size() == 1)
        {
            auto& child = ast.Get(node.children[0]);
            if (child.data == "NAME_IDENTIFIER")
            {
                code.push_back(Instruction{.type=Instruction::TYPE::LOAD, .id_params={node.id, node.children[0]}, .str_param=child.data2});
            }
            else
                code.push_back(Instruction{.type=Instruction::TYPE::MOVE, .id_params={node.children[0], node.id}});
        }
        else if (node.data == "#unary" && node.children.size() == 1)
        {
            code.push_back(Instruction{.type=Instruction::TYPE::MOVE, .id_params={node.children[0], node.id}});
        }
        else if (node.data == "#factor")
        {
            code.push_back(Instruction{.type=Instruction::TYPE::MOVE, .id_params={node.children[0], node.id}});
            for(int i=1; i<node.children.size(); i+=2)
            {
                auto& op = ast.Get(ast.Get(node.children[i]).children[0]);
                //auto& value = ast.Get(node.children[i+1]);

                Instruction inst;

                if (op.data == "OP_MUL")
                    inst.type = Instruction::TYPE::MULMOVE;
                else if (op.data == "OP_DIV")
                    inst.type = Instruction::TYPE::DIVMOVE;

                inst.id_params = {node.children[i+1], node.id};
                code.push_back(inst);
            }
        }
        else if (node.data == "#term")
        {
            code.push_back(Instruction{.type=Instruction::TYPE::MOVE, .id_params={node.children[0], node.id}});
            for(int i=1; i<node.children.size(); i+=2)
            {
                auto& op = ast.Get(ast.Get(node.children[i]).children[0]);
                //auto& value = ast.Get(node.children[i+1]);

                Instruction inst;

                if (op.data == "OP_PLUS")
                    inst.type = Instruction::TYPE::ADDMOVE;
                else if (op.data == "OP_MINUS")
                    inst.type = Instruction::TYPE::SUBMOVE;

                inst.id_params = {node.children[i+1], node.id};
                code.push_back(inst);
            }
        }
        else if (node.data == "#comparison" && node.children.size() == 1)
        {
            code.push_back(Instruction{.type=Instruction::TYPE::MOVE, .id_params={node.children[0], node.id}});
        }
        else if (node.data == "#expression" && node.children.size() == 1)
        {
            code.push_back(Instruction{.type=Instruction::TYPE::MOVE, .id_params={node.children[0], node.id}});
        }
        else if (node.data == "#argument_list" || node.data == "#file" || node.data == "#root");
        else if (node.data == "#var_declaration")
        {
            code.push_back(Instruction{.type=Instruction::TYPE::STORE, .id_params={node.children[0],node.children[1],node.children[3]}});
        }
        else if (node.data == "#function_call")
        {
            Instruction inst;
            for(auto child_id: node.children)
            {
                auto& child = ast.Get(child_id);
                if (child.data == "NAME_IDENTIFIER")
                {
                    inst.id_params.push_back(child.id);
                }
                if (child.data == "#argument_list")
                {
                    for(auto child2_id: child.children)
                    {
                        auto& child2 = ast.Get(child2_id);
                        if (child2.data == "COMMA")
                            continue;
                        inst.id_params.push_back(child2.id);
                        //cout << "Found param: " << child2.data << ":" << child2.data2 << endl;
                    }
                }
            }
            inst.type = Instruction::TYPE::CALL;
            code.push_back(inst);
        }
        else if (node.data == "#statement");
        else
        {
            cout << "Unknown AST node " << node.data << " with " << node.children.size() << " params!!!" << endl;
            std::abort();
        }
    }

    void Run(AST& ast)
    {
        code.clear();
        CodeGen(ast, ast.root, 0);
        CodeOptimize();
        CodeRun();
    }
};
