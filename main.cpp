#include <iostream>
#include <fstream>
#include <vector>
#include <stack>

using namespace std;

struct s_stack_item
{
	int i;                         // cislo uzlu, pozice v bitovem poli
	int x;                         // 0 - odebirame uzel, 1 - ponechavame uzel  
	int nodes;                     // pocet uzlu
	int *bit_array;                // bitove pole: 0 - uzel je vyrazen, 1 - uzel je obsazen
	vector<int> edges_state_table; // rika s kolika uzly dana hrana inciduje
} stack_item;

struct s_result
{
	int min_nodes;           // minimalni pocet uzlu   
	vector<int*> bit_arrays; // vysledna bitova pole uzlu
} result;

int main(int argc, char **argv)
{ 
	if (argc != 2) 
	{	
		cerr << "usage: " << argv[0] << " input_file" << endl;
		return 1;
	}

	ifstream fin(argv[1]);

	if (!fin) 
	{
		cerr << "No such input file." << endl;
		return 1;
	}

	vector<int> *incidence_table; // incidencni tabulka - rika s jakymi hranami uzly inciduji
	int total_nodes;		      // celkovy pocet uzlu
	stack<s_stack_item> stack;    // zasobnik 

	// nacitani ze souboru //////////////////////////////////////////////////// 
	int token, n = 1, edge = 0; 

	fin >> total_nodes;

	incidence_table = new vector<int>[total_nodes];

	for (int i = 0; i < total_nodes; i++)
	{
		for (int j = 0; j < n; j++)
		{
			fin >> token;

			if (token == 1) 
			{
				if (j == n) // jedna se o smycku
				{
					incidence_table[j].push_back(edge);
					stack_item.edges_state_table.push_back(1);
				}
				else
				{
					incidence_table[i].push_back(edge);
					incidence_table[j].push_back(edge);
					stack_item.edges_state_table.push_back(2);
				}

				edge++;
			}
		}

		for (int j = n; j < total_nodes; j++) 
		{
			fin >> token;
		}
		
		n++;
	}

	fin.close();
	///////////////////////////////////////////////////////////////////////////

	stack_item.bit_array = new int[total_nodes]; 
	for (int i = 0; i < total_nodes; i++) 
	{
		stack_item.bit_array[i] = 1;
	}

	stack_item.i = -1;
	stack_item.x = 1;
	stack_item.nodes = total_nodes;
	
	stack.push(stack_item);

	result.min_nodes = total_nodes;

	while (!stack.empty())
	{
		stack_item = stack.top();
		stack.pop();

		if (stack_item.x == 0)
		{
			// zjisteni, zda-li muze byt uzel odebran
			bool ok = true;
			for (int j = 0; j < (int)incidence_table[stack_item.i].size(); j++)
			{
				if (stack_item.edges_state_table[incidence_table[stack_item.i][j]] == 1) ok = false;
			}

			if (ok)
			{
				// zkopirovani bitoveho pole uzlu
				int *temp_bit_array = new int[total_nodes];
				for (int j = 0; j < total_nodes; j++) 
				{
					temp_bit_array[j] = stack_item.bit_array[j];
				}

				// vyrazeni uzlu
				temp_bit_array[stack_item.i] = 0;

				// u kazde hrany, ktera incidovala s odebranym uzlem, je snizen pocet inciduicich uzlu o 1
				for (int j = 0; j < (int)incidence_table[stack_item.i].size(); j++)
				{
					stack_item.edges_state_table[incidence_table[stack_item.i][j]]--;
				}

				stack_item.nodes--;

				if (stack_item.nodes < result.min_nodes) 
				{
					result.min_nodes = stack_item.nodes;
					result.bit_arrays.resize(0);
					result.bit_arrays.push_back(temp_bit_array);
				}
				else
				{
					if (stack_item.nodes == result.min_nodes)
					{
						result.bit_arrays.push_back(temp_bit_array);
					}
				}

				stack_item.i++;
				if (stack_item.i < total_nodes)
				{
					stack_item.bit_array = temp_bit_array;
					stack.push(stack_item);
					stack_item.x = 1;
					stack.push(stack_item);
				}
			}
		}
		else
		{
			stack_item.i++;
			if (stack_item.i < total_nodes)
			{
				stack.push(stack_item);
				stack_item.x = 0;
				stack.push(stack_item);
			}
		}
	}

	// vypisy na obrazovku ////////////////////////////////////////////////////
	cout << "incidencni tabulka:" << endl;
	for (int i = 0; i < total_nodes; i++)
	{
		cout << i << " --> ";
		for (int j = 0; j < (int)incidence_table[i].size(); j++)
		{
			cout << incidence_table[i][j] << " ";
		}
		cout << endl;
	}

	cout << endl << "reseni: " << endl;
	for (int i = 0; i < (int)result.bit_arrays.size(); i++)
	{
		cout << i+1 << ": ";
		for (int j = 0; j < total_nodes; j++)
		{
			cout << result.bit_arrays[i][j] << " ";
		}
		cout << endl;
	}

	cout << endl; 
	///////////////////////////////////////////////////////////////////////////

	system("PAUSE");
	
	return 0;
}