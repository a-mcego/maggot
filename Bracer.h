#pragma once

#include <vector>
#include <string>

struct Bracer
{
    static void AddBraces(vector<string>& lines)
    {
        vector<string> indentations = {""};
        for(size_t i=1; i<lines.size(); ++i)
        {
            auto& line = lines[i];
            auto& line_prev = lines[i-1];
            auto ws = LineWhitespace(line);
            auto ws_prev = LineWhitespace(lines[i-1]);

            //cout << "line " << i << ": " << ws_prev.length() << " - " << ws.length() << endl;

            if (ws == ws_prev)
            {
                lines[i-1] += ";";
            }
            else if (ws.length() > ws_prev.length())
            {
                if (!HasCommonPrefix(ws,ws_prev))
                    KILL("Inconsistent whitespace A!");
                indentations.push_back(ws);
                line_prev += "{";
            }
            else if (ws.length() < ws_prev.length())
            {
                for(int i=indentations.size()-1; i>=0; --i)
                {
                    auto& ind = indentations[i];
                    if (!HasCommonPrefix(ws, ind))
                        KILL("Inconsistent whitespace B!");
                    if (ind == ws)
                        break;
                    line_prev += ";}";
                    indentations.pop_back();
                }
                if (indentations.empty())
                    KILL("Inconsistent whitespace C!");
            }
        }
    }
};

