#define LENGTH 1000

#include "mpi.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <cmath>

using namespace std;

struct s_stack_item
{
	int i;                         // cislo uzlu, pozice v bitovem poli
	int x;                         // 0 - odebirame uzel, 1 - ponechavame uzel  
	int nodes_count;               // pocet uzlu
	int *bit_array;                // bitove pole: 0 - uzel je vyrazen, 1 - uzel je obsazen
	vector<int> edges_state_table; // rika s kolika uzly dana hrana inciduje
} stack_item;

struct s_result
{
	int nodes_min_count;           // minimalni pocet uzlu   
	vector<int*> bit_arrays;       // vysledna bitova pole uzlu
} result;

vector<int> *incidence_table;      // incidencni tabulka - rika s jakymi hranami uzly inciduji
int nodes_total_count;             // celkovy pocet uzlu
int edges_total_count;             // celkovy pocet hran
stack<s_stack_item> s;             // zasobnik 
vector<s_stack_item> v1, v2;       // vektory pro rozdeleni prace

int my_rank, p;

int loadData(char *file)
{
	ifstream fin(file);

	if (!fin) 
	{
		cerr << "No such input file." << endl;
		return 1;
	}

	int token, n = 1, edge = 0; 

	fin >> nodes_total_count;

	incidence_table = new vector<int>[nodes_total_count];

	for (int i = 0; i < nodes_total_count; i++)
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

		for (int j = n; j < nodes_total_count; j++) 
		{
			fin >> token;
		}
		
		n++;
	}

	fin.close();

	edges_total_count = (int)stack_item.edges_state_table.size();

	return 0;
}

void sendIncidenceTable()
{
	int size = nodes_total_count + 2;
	for (int i = 0; i < nodes_total_count; i++)
	{
		for (int j = 0; j < (int)incidence_table[i].size(); j++)
		{
			++size;
		}
	}

	int *incidence_table_message = new int[size];

	int k = 0;
	incidence_table_message[k++] = nodes_total_count;
	incidence_table_message[k++] = size;
	for (int i = 0; i < nodes_total_count; i++)
	{
		for (int j = 0; j < (int)incidence_table[i].size(); j++)
		{
			incidence_table_message[k++] = incidence_table[i][j];
		}
		incidence_table_message[k++] = -1;
	}

	//MPI_Send(incidence_table_message, size, MPI_INT, 1, 0, MPI_COMM_WORLD);

	MPI_Request *request = new MPI_Request;

	for (int i = 1; i < p; i++)
	{
		MPI_Isend(incidence_table_message, size, MPI_INT, i, 0, MPI_COMM_WORLD, request);
	}

	/*
	cout << "incidencni tabulka:" << endl;
	for (int i = 0; i < nodes_total_count; i++)
	{
		cout << i << " --> ";
		for (int j = 0; j < (int)incidence_table[i].size(); j++)
		{
			cout << incidence_table[i][j] << " ";
		}
		cout << endl;
	}
	*/

	cout << "* proces " << my_rank << " rozeslal incidencni tabulku " << p-1 << " procesum" << endl;
}

void receiveIncidenceTable()
{
	int message[LENGTH];
	MPI_Status status;

	MPI_Recv(message, LENGTH, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

	incidence_table = new vector<int>[message[0]];

	int j = 0;
	for (int i = 2; i < message[1]; i++)
	{
		if (message[i] != -1)
		{
			incidence_table[j].push_back(message[i]);
		}
		else j++;
	}

	/*
	for (int i = 0; i < message[0]; i++)
	{
		cout << i << " --> ";
		for (int j = 0; j < (int)incidence_table[i].size(); j++)
		{
			cout << incidence_table[i][j] << " ";
		}
		cout << endl;
	}
	*/

	cout << "* proces " << my_rank << " prijal incidencni tabulku" << endl;
}

void sendWork(vector<s_stack_item> v)
{
	int size = 5 + nodes_total_count + edges_total_count;
	
	for (int i = 0; i < p-1; i++)
	{
		int *stack_item_message = new int[size];

		stack_item_message[0] = v[i].i;
		stack_item_message[1] = v[i].x;
		stack_item_message[2] = v[i].nodes_count;
		stack_item_message[3] = nodes_total_count;
		for (int j = 0; j < nodes_total_count; j++)
		{
			stack_item_message[4+j] = v[i].bit_array[j]; 
		}
		stack_item_message[4+nodes_total_count] = edges_total_count;
		for (int j = 0; j < edges_total_count; j++)
		{
			stack_item_message[5+nodes_total_count+j] = v[i].edges_state_table[j]; 
		}

		MPI_Request *request = new MPI_Request;
		MPI_Isend(stack_item_message, size, MPI_INT, i+1, 1, MPI_COMM_WORLD, request);

		////////////////////////////////////////////////////////////////////////////////////

		/*
		cout << "* prace pro proces " << i+1 << ": " << endl;
		cout << "i: " << v[i].i << "  x: " << v[i].x << "  pocet uzlu: " << v[i].nodes_count << endl;
		cout << "bitove pole: ";
		for (int j = 0; j < nodes_total_count; j++)
		{
			cout << v[i].bit_array[j]; 
		}
		cout << endl << "stavova tabulka hran: ";
		for (int j = 0; j < edges_total_count; j++)
		{
			cout << v[i].edges_state_table[j]; 
		}
		cout << endl << endl;
		*/
	}

	cout << "* proces " << my_rank << " rozeslal praci " << p-1 << " procesum" << endl;

	for (int i = p-1; i < (int)v.size(); i++)
	{
		s.push(v[i]);
	}
}

void receiveWork()
{
	int message[LENGTH];
	MPI_Status status;

	MPI_Recv(message, LENGTH, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);

	stack_item.i = message[0];
	stack_item.x = message[1];
	stack_item.nodes_count = message[2];
	nodes_total_count = message[3];
	stack_item.bit_array = new int[nodes_total_count]; 
	for (int j = 0; j < nodes_total_count; j++)
	{
		stack_item.bit_array[j] = message[4+j]; 
	}
	edges_total_count = message[4+nodes_total_count];
	for (int j = 0; j < edges_total_count; j++)
	{
		stack_item.edges_state_table.push_back(message[5+nodes_total_count+j]); 
	}

	////////////////////////////////////////////////////////////////////////////////////

	/*
	cout << endl << "i: " << stack_item.i << "  x: " << stack_item.x << "  pocet uzlu: " << stack_item.nodes_count << endl;
	cout << "bitove pole: ";
	for (int j = 0; j < nodes_total_count; j++)
	{
		cout << stack_item.bit_array[j]; 
	}
	cout << endl << "stavova tabulka hran: ";
	for (int j = 0; j < edges_total_count; j++)
	{
		cout << stack_item.edges_state_table[j]; 
	}
	cout << endl;
	*/
			
	cout << "* proces " << my_rank << " prijal praci" << endl;

	////////////////////////////////////////////////////////////////////////////////////

	s.push(stack_item);
}

void countWork2();

void countWork1()
{
	v2.clear();
	v2.resize(0);

	for (int i = 0; i < (int)v1.size(); i++)
	{
		v1[i].i++;

		//v1[i].x = 1;
		v2.push_back(v1[i]);

		//v1[i].x = 0;

		// zjisteni, zda-li muze byt uzel odebran
		bool ok = true;
		for (int j = 0; j < (int)incidence_table[v1[i].i].size(); j++)
		{
			if (v1[i].edges_state_table[incidence_table[v1[i].i][j]] == 1) ok = false;
		}

		if (ok)
		{
			// zkopirovani bitoveho pole uzlu
			int *temp_bit_array = new int[nodes_total_count];
			for (int j = 0; j < nodes_total_count; j++) 
			{
				temp_bit_array[j] = v1[i].bit_array[j];
			}

			// vyrazeni uzlu
			temp_bit_array[v1[i].i] = 0;

			// u kazde hrany, ktera incidovala s odebranym uzlem, je snizen pocet inciduicich uzlu o 1
			for (int j = 0; j < (int)incidence_table[v1[i].i].size(); j++)
			{
				v1[i].edges_state_table[incidence_table[v1[i].i][j]]--;
			}

			v1[i].nodes_count--;

			if (v1[i].nodes_count < result.nodes_min_count) 
			{
				result.nodes_min_count = v1[i].nodes_count;
				result.bit_arrays.resize(0);
				result.bit_arrays.push_back(temp_bit_array);
			}
			else
			{
				if (v1[i].nodes_count == result.nodes_min_count)
				{
					result.bit_arrays.push_back(temp_bit_array);
				}
			}

			v1[i].bit_array = temp_bit_array;
			v2.push_back(v1[i]);
		}
	}

	if ((int)v2.size() >= p) sendWork(v2);
	else countWork2();
}

void countWork2()
{
	v1.clear();
	v1.resize(0);

	for (int i = 0; i < (int)v2.size(); i++)
	{
		v2[i].i++;

		//v2[i].x = 1;
		v1.push_back(v2[i]);

		//v2[i].x = 0;

		// zjisteni, zda-li muze byt uzel odebran
		bool ok = true;
		for (int j = 0; j < (int)incidence_table[v2[i].i].size(); j++)
		{
			if (v2[i].edges_state_table[incidence_table[v2[i].i][j]] == 1) ok = false;
		}

		if (ok)
		{
			// zkopirovani bitoveho pole uzlu
			int *temp_bit_array = new int[nodes_total_count];
			for (int j = 0; j < nodes_total_count; j++) 
			{
				temp_bit_array[j] = v2[i].bit_array[j];
			}

			// vyrazeni uzlu
			temp_bit_array[v2[i].i] = 0;

			// u kazde hrany, ktera incidovala s odebranym uzlem, je snizen pocet inciduicich uzlu o 1
			for (int j = 0; j < (int)incidence_table[v2[i].i].size(); j++)
			{
				v2[i].edges_state_table[incidence_table[v2[i].i][j]]--;
			}

			v2[i].nodes_count--;

			if (v2[i].nodes_count < result.nodes_min_count) 
			{
				result.nodes_min_count = v2[i].nodes_count;
				result.bit_arrays.resize(0);
				result.bit_arrays.push_back(temp_bit_array);
			}
			else
			{
				if (v2[i].nodes_count == result.nodes_min_count)
				{
					result.bit_arrays.push_back(temp_bit_array);
				}
			}

			v2[i].bit_array = temp_bit_array;
			v1.push_back(v2[i]);
		}
	}

	if ((int)v1.size() >= p) sendWork(v1);
	else countWork1();
}

void devideAndSendWork()
{
	stack_item.bit_array = new int[nodes_total_count]; 
	for (int i = 0; i < nodes_total_count; i++) 
	{
		stack_item.bit_array[i] = 1;
	}
	stack_item.i = -1;
	stack_item.x = 1;
	stack_item.nodes_count = nodes_total_count;

	result.nodes_min_count = nodes_total_count;	
	
	/////////////////////////////////////////////////////
	/*
	int depth = (int)ceil(log((float)p)/log(2.f));
	cout << depth << endl;
	*/

	v1.push_back(stack_item);
	countWork1();
}

void sendCurrentResult()
{
	int size = 3 + (int)result.bit_arrays.size() * nodes_total_count;
	
	for (int i = 0; i < p-1; i++)
	{
		int *current_result_message = new int[size];

		current_result_message[0] = result.nodes_min_count;
		current_result_message[1] = (int)result.bit_arrays.size();
		current_result_message[2] = nodes_total_count;
		int m = 3;
		for (int j = 0; j < (int)result.bit_arrays.size(); j++)
			for (int k = 0; k < nodes_total_count; k++)
			{
				current_result_message[m++] = result.bit_arrays[j][k]; 
			}
		
		MPI_Request *request = new MPI_Request;
		MPI_Isend(current_result_message, size, MPI_INT, i+1, 2, MPI_COMM_WORLD, request);
	}
	
	////////////////////////////////////////////////////////////////////////////////////

	/*
	cout << "* stavajici reseni:" << endl;
	cout << "minimum uzlu: " << result.nodes_min_count << endl;
	for (int i = 0; i < (int)result.bit_arrays.size(); i++)
	{
		cout << i+1 << ": ";
		for (int j = 0; j < nodes_total_count; j++)
		{
			cout << result.bit_arrays[i][j] << " ";
		}
		cout << endl;
	}
	*/

	cout << "* proces " << my_rank << " rozeslal stavajici reseni " << p-1 << " procesum" << endl;
}

void receiveCurrentResult()
{
	int message[LENGTH];
	MPI_Status status;

	MPI_Recv(message, LENGTH, MPI_INT, 0, 2, MPI_COMM_WORLD, &status);

	result.nodes_min_count = message[0];
	int m = 3;
	for (int j = 0; j < message[1]; j++)
	{
		int *result_array = new int[message[2]];
		for (int k = 0; k < message[2]; k++)
		{
			result_array[k] = message[m++]; 
		}
		result.bit_arrays.push_back(result_array);
	}

	////////////////////////////////////////////////////////////////////////////////////

	/*
	cout << "* stavajici reseni:" << endl;
	cout << "minimum uzlu: " << result.nodes_min_count << endl;
	for (int i = 0; i < (int)result.bit_arrays.size(); i++)
	{
		cout << i+1 << ": ";
		for (int j = 0; j < nodes_total_count; j++)
		{
			cout << result.bit_arrays[i][j] << " ";
		}
		cout << endl;
	}
	*/
			
	cout << "* proces " << my_rank << " prijal stavajici reseni" << endl;
}

void count()
{
	while (!s.empty())
	{
		stack_item = s.top();
		s.pop();

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
				int *temp_bit_array = new int[nodes_total_count];
				for (int j = 0; j < nodes_total_count; j++) 
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

				stack_item.nodes_count--;

				if (stack_item.nodes_count < result.nodes_min_count) 
				{
					result.nodes_min_count = stack_item.nodes_count;
					result.bit_arrays.resize(0);
					result.bit_arrays.push_back(temp_bit_array);
				}
				else
				{
					if (stack_item.nodes_count == result.nodes_min_count)
					{
						result.bit_arrays.push_back(temp_bit_array);
					}
				}

				stack_item.i++;
				if (stack_item.i < nodes_total_count)
				{
					stack_item.bit_array = temp_bit_array;
					s.push(stack_item);
					stack_item.x = 1;
					s.push(stack_item);
				}
			}
		}
		else
		{
			if ((stack_item.nodes_count - (nodes_total_count - (stack_item.i + 1))) <= result.nodes_min_count) // orez
			{
				stack_item.i++;
				if (stack_item.i < nodes_total_count)
				{
					s.push(stack_item);
					stack_item.x = 0;
					s.push(stack_item);
				}
			}
		}
	}

	// vypisy na obrazovku ////////////////////////////////////////////////////
	/*
	cout << "incidencni tabulka:" << endl;
	for (int i = 0; i < nodes_total_count; i++)
	{
		cout << i << " --> ";
		for (int j = 0; j < (int)incidence_table[i].size(); j++)
		{
			cout << incidence_table[i][j] << " ";
		}
		cout << endl;
	}
	*/

	cout << endl << "reseni(proces " << my_rank << "): " << endl;
	cout << "minimum uzlu: " << result.nodes_min_count << endl;
	cout << "pocet reseni: " << (int)result.bit_arrays.size() << endl;
	/*
	for (int i = 0; i < (int)result.bit_arrays.size(); i++)
	{
		cout << i+1 << ": ";
		for (int j = 0; j < nodes_total_count; j++)
		{
			cout << result.bit_arrays[i][j] << " ";
		}
		cout << endl;
	}
	cout << endl;
	*/
	///////////////////////////////////////////////////////////////////////////
}

int main(int argc, char **argv)
{ 
	/*
	if (argc != 2) 
	{	
		cerr << "usage: " << argv[0] << " input_file" << endl;
		return 1;
	}
	*/

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	if (my_rank == 0)
	{
		if (loadData("in.txt")) return 1;
		sendIncidenceTable();
		devideAndSendWork();
		sendCurrentResult();
		count();
	}
	else
	{
		receiveIncidenceTable();
		receiveWork();
		receiveCurrentResult();
		count();
	}

	MPI_Finalize();
	
	return 0;
}