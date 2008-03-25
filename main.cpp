#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

vector<int> *incidence_table; // incidencni tabulka - rika s jakymi hranami uzly inciduji
vector<int*> result;		  // vysledek - bitova pole uzlu
int nodes_total_count;		  // celkovy pocet uzlu
int nodes_min_count;          // minimalni pocet uzlu uzloveho pokryti

void RecursiveCount(int i, int x, int *bit_array, vector<int> edges_state_table)
{
	if (i < nodes_total_count)
	{
		if (x == 0)
		{
			// zjisteni, zda-li muze byt uzel odebran
			bool ok = true;
			for (int j = 0; j < (int)incidence_table[i].size(); j++)
			{
				if (edges_state_table[incidence_table[i][j]] == 1) ok = false;
			}

			if (ok)
			{
				// zkopirovani bitoveho pole uzlu
				int *temp_bit_array = new int[nodes_total_count];
				for (int j = 0; j < nodes_total_count; j++) 
				{
					temp_bit_array[j] = bit_array[j];
				}

				// vyrazeni uzlu
				temp_bit_array[i] = 0;

				// u kazde hrany, ktera incidovala s odebranym uzlem, je snizen pocet inciduicich uzlu o 1
				for (int j = 0; j < (int)incidence_table[i].size(); j++)
				{
					edges_state_table[incidence_table[i][j]]--;
				}

				int nmc = 0;
				for (int j = 0; j < nodes_total_count; j++) 
				{
					nmc += temp_bit_array[j];
				}

				if (nmc < nodes_min_count) 
				{
					nodes_min_count = nmc;
					result.resize(0);
					result.push_back(temp_bit_array);
				}
				else
				{
					if (nmc == nodes_min_count)
					{
						result.push_back(temp_bit_array);
					}
				}

				i++;
				RecursiveCount(i, 0, temp_bit_array, edges_state_table);
				RecursiveCount(i, 1, temp_bit_array, edges_state_table);
			}
		}
		else
		{
			i++;
			RecursiveCount(i, 0, bit_array, edges_state_table);
			RecursiveCount(i, 1, bit_array, edges_state_table);
		}
	}
}

int main(int argc, char *argv[])
{ 
	/*
	char infile[256];
	cout << "Input file: ";
    cin.getline(infile, 256);
	ifstream fin(infile);
	*/

	ifstream fin("in.txt");

	if (!fin) 
	{
		cerr << "Chyba. Nelze otevrit vstupni soubor." << endl;
		return 1;
	}

	int nx = 1, edge = 0;
	char token; 
	vector<int> edges_state_table;

	fin >> nodes_total_count;

	incidence_table = new vector<int>[nodes_total_count];

	for (int i = 0; i < nodes_total_count; i++)
	{
		for (int j = 0; j < nx; j++)
		{
			fin >> token;

			if (token == '1') 
			{
				if (j == nx) // jedna se o smycku
				{
					incidence_table[j].push_back(edge);
					edges_state_table.push_back(1);
				}
				else
				{
					incidence_table[i].push_back(edge);
					incidence_table[j].push_back(edge);
					edges_state_table.push_back(2);
				}

				edge++;
			}
		}

		for (int j = nx; j < nodes_total_count; j++) 
		{
			fin >> token;
		}
		
		nx++;
	}

	fin.close();

	// bitove pole uzlu
	int *bit_array = new int[nodes_total_count]; 
    // nastaveni bitoveho pole na same 1
	for (int i = 0; i < nodes_total_count; i++) 
	{
		bit_array[i] = 1;
	}
	
	nodes_min_count = nodes_total_count;

	RecursiveCount(0, 0, bit_array, edges_state_table);
	RecursiveCount(0, 1, bit_array, edges_state_table);

	// vypisy na obrazovku ///////////////////////////////////////

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

	cout << endl << "reseni: " << endl;
	for (int i = 0; i < (int)result.size(); i++)
	{
		cout << i+1 << ": ";
		for (int j = 0; j < nodes_total_count; j++)
		{
			cout << result[i][j] << " ";
		}
		cout << endl;
	}

	cout << endl; system("PAUSE");

	return 0;
}