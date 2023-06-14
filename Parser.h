#pragma once
#include "RunningID.h"
#include "Util.h"

enum struct DET
{
    N1,
    N01,
    N0INF,
    N1INF,
    NONE,
    N
};

struct
{
    int min{},max{};

    bool OverLimit(int amount)
    {
        if(max == -1)
            return false;
        return amount >= max;
    }

} detlimits[int(DET::N)] =
{
    {1,1},
    {0,1},
    {0,-1},
    {1,-1},
    {0,0},
};

struct AST
{
    struct Node
    {
        RID id{RunningID<RID>::Next()};
        vector<RID> children;
        string data, data2;
    };

    vector<Node> nodes;
    RID root{-1};

    void Finalize()
    {
        map<RID,RID> id_mapping;
        for(int i=0; i<nodes.size(); ++i)
            id_mapping[nodes[i].id] = RID{i};

        for(Node& node: nodes)
        {
            node.id = id_mapping[node.id];
            for(RID& child: node.children)
                child = id_mapping[child];
        }
        root = id_mapping[root];
    }

    RID NewNode()
    {
        nodes.push_back(Node());
        return nodes.back().id;
    }
    Node empty{};
    Node& GetSlow(RID r)
    {
        for(Node& n: nodes)
            if (n.id == r)
                return n;
        cout << __PRETTY_FUNCTION__ << " ERROR" << endl;
        return empty;
    }
    Node& Get(RID r)
    {
        return nodes[r.toT()];
    }
};

struct Grammar
{
    AST ast;
    //chunk delimited by () in the grammar
    struct ChunkToken
    {
        string name;
        bool ignore{false};
    };

    struct Chunk
    {
        vector<ChunkToken> tokens;
        DET type{}; //repeat? * N0INF,  + N1INF,  ? N01, and without anything it's N1, exactly 1 times
    };
    //one alternative definition of the current non-terminal
    struct Definition
    {
        string name;
        vector<Chunk> chunks;
    };

    map<string,vector<Definition>> definitions;

    void LoadDefaultGrammar() //This loads the default grammar grammar so we can read the actual grammar grammar
    {
        *this = Grammar();
        definitions["num_determiner"].push_back(Definition{"num_determiner",{Chunk{{ChunkToken{"N0INF"}},DET::N1}}});
        definitions["num_determiner"].push_back(Definition{"num_determiner",{Chunk{{ChunkToken{"N1INF"}},DET::N1}}});
        definitions["num_determiner"].push_back(Definition{"num_determiner",{Chunk{{ChunkToken{"N01"}},DET::N1}}});

        definitions["def_elem"].push_back(Definition{"def_elem",{Chunk{{ChunkToken{"token"}},DET::N1}}});
        definitions["def_elem"].push_back(Definition{"def_elem",{Chunk{{ChunkToken{"LEFT"}},DET::N1},Chunk{{ChunkToken{"def_elem"}},DET::N1INF},Chunk{{ChunkToken{"RIGHT"},ChunkToken{"num_determiner"}},DET::N1}}});

        definitions["definition"].push_back(Definition{"definition",{Chunk{{ChunkToken{"def_elem"}},DET::N1INF}}});

        definitions["line"].push_back(Definition{"line",{Chunk{{ChunkToken{"NONTERMINAL"},ChunkToken{"EQ"},ChunkToken{"definition"}},DET::N1}}});

        definitions["token"].push_back(Definition{"token",{Chunk{{ChunkToken{"TERMINAL"}},DET::N1}}});
        definitions["token"].push_back(Definition{"token",{Chunk{{ChunkToken{"NONTERMINAL"}},DET::N1}}});

        definitions["root"].push_back(Definition{"root",{Chunk{{ChunkToken{"line"},ChunkToken{"LINEFEED"}},DET::N1INF}}});
    }

    bool IsTerminal(const ChunkToken& t)
    {
        if (t.name[0] == '!')
            return (t.name[1] >= 'A' && t.name[1] <= 'Z');
        return (t.name[0] >= 'A' && t.name[0] <= 'Z');
    }

    struct ParseResult
    {
        RID rid{-1}; //ID of the AST node
        int token_pos{0}; //what is the new position in the tokens array
    };

    ParseResult Recurse(vector<Token>& tokens, int start_token_position, ChunkToken considered)
    {
        auto& defs = definitions[considered.name];
        //cout << start_token_position << ": " << considered.name << endl;
        //go through all definitions, try them one by one
        for (u64 def_n=0; def_n<defs.size(); ++def_n)
        {
            int def_token_position = start_token_position;
            //cout << "init chunk_token_position to: " << def_token_position << endl;
            RID astnode_id = ast.NewNode();

            auto& currdef = defs[def_n];
            bool chunks_successful{true};
            [&]()->void
            {
                //cout << "Start chunk loop. def " << def_n << " has " << currdef.chunks.size() << " chunks." << endl;
                for(u64 chunk_n=0; chunk_n<currdef.chunks.size(); ++chunk_n)
                {
                    auto& chunk = currdef.chunks[chunk_n];

                    auto& limits = detlimits[(int)chunk.type];
                    int chunk_token_position = def_token_position;
                    //cout << "init chunk_token_position to: " << chunk_token_position << endl;

                    //cout << "Chunk " << chunk_n << " limits: " << limits.min << "|" << limits.max << endl;
                    int chunk_repeats=0;
                    vector<RID> chunk_results;

                    while(true)
                    {
                        if (limits.OverLimit(chunk_repeats))
                        {
                            break;
                        }
                        vector<RID> chunk_repeat_results;
                        bool result_good{true};
                        int token_token_position = chunk_token_position;
                        //cout << "init token_token_position to: " << token_token_position << endl;
                        //one chunk run
                        [&]()->void
                        {
                            //cout << "Start token loop. chunk " << def_n << ":" << chunk_n << " has " << chunk.tokens.size() << " tokens." << endl;
                            for(u64 token_n=0; token_n<chunk.tokens.size(); ++token_n)
                            {
                                Token& inputtoken = tokens[token_token_position];
                                auto& grammar_token = chunk.tokens[token_n];
                                //cout << "pos=" << token_token_position << ", rep=" << chunk_repeat_results.size() << " " << def_n << ":" << chunk_n << ":" << token_n << " Input token: " << inputtoken.type << ":" << inputtoken.data << ", expected: " << grammar_token << endl;
                                if (!IsTerminal(grammar_token))
                                {
                                    //Forward track
                                    auto [recret,newpos] = Recurse(tokens, token_token_position, grammar_token);
                                    if (recret == RID{-1})
                                    {
                                        chunk_repeat_results.clear();
                                        result_good = false;
                                        return;
                                    }
                                    token_token_position = newpos;
                                    //cout << "Came back " << grammar_token.ignore << "?" << grammar_token.name << "! " << token_token_position << endl;
                                    if (!grammar_token.ignore)
                                        chunk_repeat_results.push_back(recret);
                                }
                                else
                                {
                                    if (inputtoken.type == grammar_token.name)
                                    {
                                        //cout << "Match " << grammar_token.ignore << "?" << grammar_token.name << "! " << token_token_position << endl;

                                        ++token_token_position;
                                        //cout << "token_token_position now " << token_token_position << endl;
                                        //make AST node here

                                        AST::Node& tokennode = ast.GetSlow(ast.NewNode());
                                        tokennode.data = grammar_token.name;
                                        tokennode.data2 = inputtoken.data;
                                        if (!grammar_token.ignore)
                                            chunk_repeat_results.push_back(tokennode.id);
                                    }
                                    else
                                    {
                                        //cout << "No match " << grammar_token.name << "! " << token_token_position << endl;
                                        chunk_repeat_results.clear();
                                        result_good = false;
                                        return;
                                    }
                                }
                            }
                            //cout << "Stop token loop." << endl;
                        }();

                        //if (!chunk_repeat_results.empty())
                        if (result_good)
                        {
                            //cout << "set chunk_token_position from " << chunk_token_position << " to " << token_token_position << endl;
                            ++chunk_repeats;
                            chunk_token_position = token_token_position;
                            chunk_results.insert(chunk_results.end(), chunk_repeat_results.begin(), chunk_repeat_results.end());
                            if (chunk_token_position >= tokens.size())
                            {
                                break;
                            }
                        }
                        else
                            break;
                    }
                    //cout << "Found " << chunk_repeats << " chunk_repeats." << endl;

                    if (chunk_repeats < limits.min)
                    {
                        chunks_successful = false;
                        chunk_results.clear();
                        return;
                    }
                    else
                    {
                        //cout << "set def_token_position from " << def_token_position << " to " << chunk_token_position << endl;
                        ast.GetSlow(astnode_id).children.insert(ast.GetSlow(astnode_id).children.end(), chunk_results.begin(), chunk_results.end());
                        def_token_position = chunk_token_position;
                        if (def_token_position >= tokens.size())
                            return;
                    }
                }
            }();

            //RID parse_result = ParseDef(currdef,token_position)
            auto& curr_astnode = ast.GetSlow(astnode_id);
            if (!chunks_successful)
            {
                //cout << "Chunk not successful :(" << endl;
                curr_astnode.children.clear();
                continue;
            }
            curr_astnode.data = "#" + considered.name;
            return {astnode_id, def_token_position};
        }
        //cout << __PRETTY_FUNCTION__ << ": Parse " << considered << " failed." << endl;
        return {RID{-1},1000000000};
    }

    int amount_ns=0;
    void TraverseAST(RID id, int amount_spaces)
    {
        //post-traversal
        AST::Node& node = ast.Get(id);
        for(auto& child: node.children)
            TraverseAST(child, amount_spaces+1);
        cout << string(amount_spaces,' ') << node.id << ":" << node.data;
        if (!node.data2.empty())
            cout << ":" << node.data2;
        cout << " ! ";
        for(auto& child: node.children)
            cout << child << " ";
        cout << endl;
    }

    void LoadGrammarFromAST(AST& ast)
    {
        *this = Grammar();
        AST::Node& root = ast.Get(ast.root);
        for(RID root_child: root.children)
        {
            AST::Node& line = ast.Get(root_child);
            if (line.data != "#line")
                continue;

            if (line.children.size() != 3)
                continue;

            AST::Node& line_child0 = ast.Get(line.children[0]);
            if (line_child0.data != "NONTERMINAL")
                continue;

            Definition def;
            def.name = line_child0.data2;

            AST::Node& definition = ast.Get(line.children[2]);
            for(auto& def_elem_id: definition.children)
            {
                AST::Node& def_elem = ast.Get(def_elem_id);
                if (def_elem.children.size() == 1 || def_elem.children.size() == 2)
                {
                    AST::Node& token = ast.Get(def_elem.children.back());
                    AST::Node& token_below = ast.Get(token.children[0]);
                    def.chunks.push_back(Chunk{{ChunkToken{token_below.data2,def_elem.children.size() == 2}},DET::N1});
                }
                else if (def_elem.children.size() >= 4)
                {
                    Chunk c;
                    for(int i=1; i<def_elem.children.size()-2; ++i)
                    {
                        AST::Node& inner_def_elem = ast.Get(def_elem.children[i]);
                        AST::Node& token = ast.Get(inner_def_elem.children.back());
                        AST::Node& token_below = ast.Get(token.children[0]);
                        c.tokens.push_back(ChunkToken{token_below.data2, inner_def_elem.children.size()==2});
                    }


                    AST::Node& determiner = ast.Get(def_elem.children.back());
                    AST::Node& determiner_below = ast.Get(determiner.children[0]);
                    string determiner_type = determiner_below.data;

                    if (determiner_type == "N1")
                        c.type = DET::N1;
                    else if (determiner_type == "N01")
                        c.type = DET::N01;
                    else if (determiner_type == "N0INF")
                        c.type = DET::N0INF;
                    else if (determiner_type == "N1INF")
                        c.type = DET::N1INF;
                    else
                    {
                        cout << "Weird determiner type " << determiner_type << "." << endl;
                    }
                    def.chunks.push_back(c);
                }
            }
            definitions[def.name].push_back(def);
            //cout << "Line data: token name " << def.name << " with " << definition.children.size() << " def_elems." << endl;
        }
    }

    void Parse(vector<Token> tokens)
    {
        auto ret = Recurse(tokens,0,ChunkToken{"root",false});
        ast.root = ret.rid;
        if (ret.rid == RID{-1})
        {
            cout << "Failed parse. :(" << endl;
            std::abort();
        }
        ast.Finalize();
        //TraverseAST(ast.root, 0);
    }
};
