#define LENGTH 10000
#define CHECK_MSG_AMOUNT 100

#define MSG_INCIDANCE_TABLE 0
#define MSG_INIT_WORK 1
#define MSG_INIT_RESULT 2
#define MSG_REQUEST 3
#define MSG_WORK 4
#define MSG_IDLE 5
#define MSG_FINAL_RESULT 6
#define MSG_PESEK 7

#define NONE_COLOR 0
#define WHITE_COLOR 'W'
#define BLACK_COLOR 'B'

#define LOG

#include "mpi.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
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
list<s_stack_item> s;              // zasobnik 
vector<s_stack_item> v1, v2;       // vektory pro rozdeleni prace
char color = WHITE_COLOR;          // barva procesu pro peskovy algoritmus
char pesek_color = NONE_COLOR;    // barva peska k odeslani

int my_rank, p, flag, counter, idle;
int message[LENGTH];
MPI_Status status;
MPI_Request *request;

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

	for (int i = 1; i < p; i++)
	{
		MPI_Isend(incidence_table_message, size, MPI_INT, i, MSG_INCIDANCE_TABLE, MPI_COMM_WORLD, request);
	}

    #ifdef LOG
	cout << "* procesor " << my_rank << " rozeslal incidencni tabulku" << endl;
    #endif
}

void receiveIncidenceTable()
{
	MPI_Recv(message, LENGTH, MPI_INT, 0, MSG_INCIDANCE_TABLE, MPI_COMM_WORLD, &status);

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

	#ifdef LOG
	cout << "* procesor " << my_rank << " prijal incidencni tabulku" << endl;
	#endif
}

void sendInitWork(vector<s_stack_item> v)
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

		MPI_Isend(stack_item_message, size, MPI_INT, i+1, MSG_INIT_WORK, MPI_COMM_WORLD, request);
	}

	for (int i = p-1; i < (int)v.size(); i++)
	{
		s.push_front(v[i]); // zbyvajici prace pro procesor root
	}

	#ifdef LOG
	cout << "* procesor " << my_rank << " rozdelil a rozeslal praci" << endl;
	#endif
}

void receiveInitWork()
{
	MPI_Recv(message, LENGTH, MPI_INT, 0, MSG_INIT_WORK, MPI_COMM_WORLD, &status);

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

	s.push_front(stack_item);

	#ifdef LOG
	cout << "* procesor " << my_rank << " prijal praci" << endl;
	#endif
}

void countWork(vector<s_stack_item> &va, vector<s_stack_item> &vb)
{
	vb.clear();
	vb.resize(0);

	for (int i = 0; i < (int)va.size(); i++)
	{
		va[i].i++;

		vb.push_back(va[i]);

		// zjisteni, zda-li muze byt uzel odebran
		bool ok = true;
		for (int j = 0; j < (int)incidence_table[va[i].i].size(); j++)
		{
			if (va[i].edges_state_table[incidence_table[va[i].i][j]] == 1) ok = false;
		}

		if (ok)
		{
			// zkopirovani bitoveho pole uzlu
			int *temp_bit_array = new int[nodes_total_count];
			for (int j = 0; j < nodes_total_count; j++) 
			{
				temp_bit_array[j] = va[i].bit_array[j];
			}

			// vyrazeni uzlu
			temp_bit_array[va[i].i] = 0;

			// u kazde hrany, ktera incidovala s odebranym uzlem, je snizen pocet inciduicich uzlu o 1
			for (int j = 0; j < (int)incidence_table[va[i].i].size(); j++)
			{
				va[i].edges_state_table[incidence_table[va[i].i][j]]--;
			}

			va[i].nodes_count--;

			if (va[i].nodes_count < result.nodes_min_count) 
			{
				result.nodes_min_count = va[i].nodes_count;
				result.bit_arrays.resize(0);
				result.bit_arrays.push_back(temp_bit_array);
			}
			else
			{
				if (va[i].nodes_count == result.nodes_min_count)
				{
					result.bit_arrays.push_back(temp_bit_array);
				}
			}

			va[i].bit_array = temp_bit_array;
			vb.push_back(va[i]);
		}
	}

	if ((int)vb.size() >= p) sendInitWork(vb);
	else countWork(vb, va);
}

void sendInitResult()
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
		
		MPI_Isend(current_result_message, size, MPI_INT, i+1, MSG_INIT_RESULT, MPI_COMM_WORLD, request);
	}

	#ifdef LOG
	cout << "* procesor " << my_rank << " rozeslal stavajici reseni" << endl;
	#endif
}

void receiveInitResult()
{
	MPI_Recv(message, LENGTH, MPI_INT, 0, MSG_INIT_RESULT, MPI_COMM_WORLD, &status);

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
		
	#ifdef LOG
	cout << "* procesor " << my_rank << " prijal stavajici reseni" << endl;
	#endif
}

void sendFinalResult()
{
	int size = 3 + (int)result.bit_arrays.size() * nodes_total_count;
	
	int *final_result_message = new int[size];

	final_result_message[0] = result.nodes_min_count;
	final_result_message[1] = (int)result.bit_arrays.size();
	final_result_message[2] = nodes_total_count;
	int k = 3;
	for (int i = 0; i < (int)result.bit_arrays.size(); i++)
		for (int j = 0; j < nodes_total_count; j++)
	{
		final_result_message[k++] = result.bit_arrays[i][j]; 
	}
		
	MPI_Isend(final_result_message, size, MPI_INT, my_rank+1, MSG_FINAL_RESULT, MPI_COMM_WORLD, request);

	#ifdef LOG
	cout << "* procesor " << my_rank << " poslal finalni reseni procesoru " << my_rank+1 << endl;
	#endif
}

void receiveFinalResult()
{
	MPI_Recv(message, LENGTH, MPI_INT, p-1, MSG_FINAL_RESULT, MPI_COMM_WORLD, &status);

	s_result result_final;

	result_final.nodes_min_count = message[0];
	int k = 3;
	for (int i = 0; i < message[1]; i++)
	{
		int *result_array = new int[message[2]];
		for (int j = 0; j < message[2]; j++)
		{
			result_array[j] = message[k++]; 
		}
		result_final.bit_arrays.push_back(result_array);
	}

	result = result_final;

	#ifdef LOG
	cout << "* procesor " << my_rank << " prijal finalni reseni" << endl;
	#endif
}

void receiveAndResendFinalResult()
{
	MPI_Recv(message, LENGTH, MPI_INT, MPI_ANY_SOURCE, MSG_FINAL_RESULT, MPI_COMM_WORLD, &status);

	s_result result_temp;

	result_temp.nodes_min_count = message[0];
	int k = 3;
	for (int i = 0; i < message[1]; i++)
	{
		int *result_array = new int[message[2]];
		for (int j = 0; j < message[2]; j++)
		{
			result_array[j] = message[k++]; 
		}
		result_temp.bit_arrays.push_back(result_array);
	}

	int *final_result_message, size;

	if (result_temp.nodes_min_count < result.nodes_min_count)
	{
		size = 3 + (int)result_temp.bit_arrays.size() * nodes_total_count;
		final_result_message = new int[size];

		for (int i = 0; i < size; i++)
		{
			final_result_message[i] = message[i];
		}
	}
	else
	{
		if (result_temp.nodes_min_count > result.nodes_min_count)
		{
			size = 3 + (int)result.bit_arrays.size() * nodes_total_count;
		    final_result_message = new int[size];

			final_result_message[0] = result.nodes_min_count;
			final_result_message[1] = (int)result.bit_arrays.size();
			final_result_message[2] = nodes_total_count;
			int k = 3;
			for (int i = 0; i < (int)result.bit_arrays.size(); i++)
				for (int j = 0; j < nodes_total_count; j++)
			{
				final_result_message[k++] = result.bit_arrays[i][j]; 
			}
		}
		else
		{
			for (int i = 0; i < (int)result.bit_arrays.size(); i++)
			{
				int *result_array = new int[nodes_total_count];
				for (int j = 0; j < nodes_total_count; j++)
				{
					result_array[j] = result.bit_arrays[i][j];
				}
				result_temp.bit_arrays.push_back(result_array);
			}

			size = 3 + (int)result_temp.bit_arrays.size() * nodes_total_count;
		    final_result_message = new int[size];

			final_result_message[0] = result_temp.nodes_min_count;
			final_result_message[1] = (int)result_temp.bit_arrays.size();
			final_result_message[2] = nodes_total_count;
			int m = 3;
			for (int j = 0; j < (int)result_temp.bit_arrays.size(); j++)
				for (int k = 0; k < nodes_total_count; k++)
			{
				final_result_message[m++] = result_temp.bit_arrays[j][k]; 
			}
		}	
	}

	if (my_rank+1 < p)
	{
		MPI_Isend(final_result_message, size, MPI_INT, my_rank+1, MSG_FINAL_RESULT, MPI_COMM_WORLD, request);
		#ifdef LOG
		cout << "* procesor " << my_rank << " prijal a preposlal finalni reseni procesoru " << my_rank+1 << endl;
		#endif
	}
	else
	{
		MPI_Isend(final_result_message, size, MPI_INT, 0, MSG_FINAL_RESULT, MPI_COMM_WORLD, request);
		#ifdef LOG
		cout << "* procesor " << my_rank << " prijal a preposlal finalni reseni procesoru " << 0 << endl;
		#endif
	}
}

void sendWork(int rank, int status)
{
	int *stack_item_message, size;

	if (status == 1)
	{
		s_stack_item stack_item_back = s.back(); // veme se nejspodnejsi polozka zasobniku
		s.pop_back();

		size = 6 + nodes_total_count + edges_total_count;
		stack_item_message = new int[size];

		stack_item_message[0] = status;
		stack_item_message[1] = stack_item_back.i;
		stack_item_message[2] = stack_item_back.x;
		stack_item_message[3] = stack_item_back.nodes_count;
		stack_item_message[4] = nodes_total_count;
		for (int j = 0; j < nodes_total_count; j++)
		{
			stack_item_message[5+j] = stack_item_back.bit_array[j]; 
		}
		stack_item_message[5+nodes_total_count] = edges_total_count;
		for (int j = 0; j < edges_total_count; j++)
		{
			stack_item_message[6+nodes_total_count+j] = stack_item_back.edges_state_table[j]; 
		}
		
		#ifdef LOG
		cout << "* procesor " << my_rank << " poslal praci procesoru " << rank << endl;
		#endif
		
		if (rank < my_rank) {
            color = BLACK_COLOR;
        }
	}
	else
	{
		size = 1;
		stack_item_message = new int;
		stack_item_message[0] = 0;

		#ifdef LOG
		cout << "* procesor " << my_rank << " neposlal praci procesoru " << rank << endl;
		#endif
	}
    
    MPI_Isend(stack_item_message, size, MPI_INT, rank, MSG_WORK, MPI_COMM_WORLD, request);
}

int receiveWork(int rank)
{
	MPI_Recv(message, LENGTH, MPI_INT, rank, MSG_WORK, MPI_COMM_WORLD, &status);

	if (message[0])
	{
		stack_item.i = message[1];
		stack_item.x = message[2];
		stack_item.nodes_count = message[3];
		nodes_total_count = message[4];
		stack_item.bit_array = new int[nodes_total_count]; 
		for (int j = 0; j < nodes_total_count; j++)
		{
			stack_item.bit_array[j] = message[5+j]; 
		}
		edges_total_count = message[5+nodes_total_count];
		stack_item.edges_state_table.clear();
		stack_item.edges_state_table.resize(0);
		for (int j = 0; j < edges_total_count; j++)
		{
			stack_item.edges_state_table.push_back(message[6+nodes_total_count+j]); 
		}

		s.push_front(stack_item);

		#ifdef LOG
		cout << "* procesor " << my_rank << " prijal praci od procesoru " << rank << endl;
		#endif

		return 1;
	}

	return 0;
}

void sendPesek() {
    int dest_rank = (my_rank + 1) % p;
    
    if (color == BLACK_COLOR) {
        pesek_color = BLACK_COLOR;
    }
    
    MPI_Isend(&pesek_color, 1, MPI_CHAR, dest_rank, MSG_PESEK, MPI_COMM_WORLD, request);
    
    #ifdef LOG
    cout << "* procesor " << my_rank << " poslal procesoru " << dest_rank << " peska " << pesek_color << endl;
	#endif
    
    color = WHITE_COLOR;
    pesek_color = NONE_COLOR;
}

void receivePesek() {
    MPI_Recv(&pesek_color, 1, MPI_CHAR, MPI_ANY_SOURCE, MSG_PESEK, MPI_COMM_WORLD, &status);
    
    #ifdef LOG
	cout << "* procesor " << my_rank << " prijal peska " << pesek_color << endl;
	#endif
    
    if (my_rank == 0) {
        if (pesek_color == WHITE_COLOR) {
            #ifdef LOG
            cout << "<<<<< ukoncuji" << endl;
            #endif
            
            sendFinalResult();
            receiveFinalResult();
            throw 2;
        } else {
            #ifdef LOG
            cout << "* procesor 0 posila nove kolo peska" << endl;
            #endif
            
            pesek_color = WHITE_COLOR;
            sendPesek();
            return;
        }
    }
    
    if (color == BLACK_COLOR) {
        pesek_color = BLACK_COLOR;
    }
}

int requestWork()
{
	int rank = my_rank;  
    
    if (my_rank == 0 && pesek_color == NONE_COLOR) {
        color = WHITE_COLOR;
        pesek_color = WHITE_COLOR;
        sendPesek();
    }
    
    
	while (1)
	{
		if (++rank == p) rank = 0;
        //if (rank == my_rank) return 0;
		if (rank == my_rank) continue;

		MPI_Isend(NULL, 0, MPI_INT, rank, MSG_REQUEST, MPI_COMM_WORLD, request);
		#ifdef LOG
		cout << "* procesor " << my_rank << " poslal zadost o praci procesoru " << rank << endl;
		#endif
        
        try {
    		while (1)
    		{
    		    if (pesek_color != NONE_COLOR) {
                    sendPesek();
                    color = WHITE_COLOR;
                }
                
                while (1) {
                    MPI_Iprobe(MPI_ANY_SOURCE, MSG_WORK, MPI_COMM_WORLD, &flag, &status);
                    
                    if (flag) {
                        if (receiveWork(status.MPI_SOURCE)) {
        					return 1;
        				} else {
        				    throw 1;//skok do vnejsiho cyklu
        				}

                    } else {
                        break;
                    }
                }
    			
    			MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    			
                // if (flag) {
    			switch (status.MPI_TAG) {
        			case MSG_REQUEST:
        				MPI_Recv(message, LENGTH, MPI_INT, status.MPI_SOURCE, MSG_REQUEST, MPI_COMM_WORLD, &status);
        				#ifdef LOG
        				cout << "* procesor " << my_rank << " prijal zadost o praci od procesoru " << status.MPI_SOURCE << endl;
        				#endif
        				sendWork(status.MPI_SOURCE, 0);
                        break;
            
        			case MSG_WORK:
        				if (receiveWork(status.MPI_SOURCE))
        				{
        					return 1;
        				}
        				else throw 1;//skok do vnejsiho cyklu
                        break;

/*
        		    case MSG_IDLE:
                        MPI_Recv(message, LENGTH, MPI_INT, status.MPI_SOURCE, MSG_IDLE, MPI_COMM_WORLD, &status);
    					#ifdef LOG
    					cout << "* procesor " << my_rank << " prijal ukoncovaci token od procesoru " << status.MPI_SOURCE << endl;
    					#endif
						sendFinalResult();
						receiveFinalResult();
                        throw 2;
                        return 0;
                        break;
*/                    
                    case MSG_FINAL_RESULT:
    					receiveAndResendFinalResult();
                        throw 10;
                        return 0;
    					break;
                    
        			case MSG_PESEK:
        			    #ifdef LOG
                        cout << "* procesor " << my_rank << " prijima peska v requestWork()" << endl;
                        #endif
                        receivePesek();
                        break;
                    
                    default:
                        #ifdef LOG
                        cout << " >>>>>>>>>> unknown message in requestWork() <<<<<<<<< " << status.MPI_TAG << endl;
                        #endif
                        break;
                }
                // }
    		}
		} catch (int i) {
		    // skok do vnejsiho cyklu
		    if (i != 1) {
                throw i;
		    }
		}
	}
}

void goToIdle()
{
	#ifdef LOG
	cout << "* procesor " << my_rank << " je idle" << endl;
	#endif

	if (my_rank != 0)
	{
        // MPI_Isend(NULL, 0, MPI_INT, 0, MSG_IDLE, MPI_COMM_WORLD, request);
        // #ifdef LOG
        // cout << "* procesor " << my_rank << " poslal ukoncovaci token procesoru 0" << endl;
        // #endif
	}
	else
	{
        pesek_color = color = WHITE_COLOR;
        
        sendPesek();
        		
		if (idle == p-1)
		{
			sendFinalResult();
			receiveFinalResult();
			return;
		}
	}
    
	counter = 0;

	while (1)
	{
		if ((++counter % CHECK_MSG_AMOUNT) == 0)
		{
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);

			if (flag)
			{
				switch (status.MPI_TAG) {
				    case MSG_REQUEST:
    					MPI_Recv(message, LENGTH, MPI_INT, status.MPI_SOURCE, MSG_REQUEST, MPI_COMM_WORLD, &status);
    					#ifdef LOG
    					cout << "* procesor " << my_rank << " prijal zadost o praci od procesoru " << status.MPI_SOURCE << endl;
    					#endif
    					sendWork(status.MPI_SOURCE, 0);
                        break;
/*
				    case MSG_IDLE:
    					MPI_Recv(message, LENGTH, MPI_INT, status.MPI_SOURCE, MSG_IDLE, MPI_COMM_WORLD, &status);
    					#ifdef LOG
    					cout << "* procesor " << my_rank << " prijal ukoncovaci token od procesoru " << status.MPI_SOURCE << endl;
    					#endif
    					if (++idle == p-1)
    					{
    						sendFinalResult();
    						receiveFinalResult();
    						return;
    					}
                        break;
*/                        
				    case MSG_FINAL_RESULT:
    					receiveAndResendFinalResult();
    					return;
    				case MSG_PESEK:
    				    #ifdef LOG
                        cout << "* procesoru " << my_rank << " prijima peska v goToIdle()" << endl;
                        #endif
                        receivePesek();
                        break;
                    default:
                        cout << " >>>>>>>>>> unknown message in goToIdle() <<<<<<<<< " << status.MPI_TAG << endl;
                        break;
    			}
			}

			counter = 0;
		}
	}
}

void probeAndHandleMessages() {
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
    
	while (flag)
	{
	    switch (status.MPI_TAG) {
	        case MSG_REQUEST:
        		MPI_Recv(message, LENGTH, MPI_INT, status.MPI_SOURCE, MSG_REQUEST, MPI_COMM_WORLD, &status);
        		#ifdef LOG
        		cout << "* procesor " << my_rank << " prijal zadost o praci od procesoru " << status.MPI_SOURCE << endl;
        		#endif

        		if ((int)s.size() > 1) sendWork(status.MPI_SOURCE, 1);
        		else sendWork(status.MPI_SOURCE, 0);
                break;
            // case MSG_IDLE:
            //     MPI_Recv(message, LENGTH, MPI_INT, status.MPI_SOURCE, MSG_IDLE, MPI_COMM_WORLD, &status);
            //     #ifdef LOG
            //     cout << "* procesor " << my_rank << " prijal ukoncovaci token od procesoru " << status.MPI_SOURCE << endl;
            //     #endif
            //     if (++idle == p-1)
            //     {
            //      sendFinalResult();
            //      receiveFinalResult();
            //      return;
            //     }
            //     break;
            case MSG_PESEK:
                #ifdef LOG
                cout << "* procesor " << my_rank << " prijima peska v probeAndHandleMessages()" << endl;
                #endif
                receivePesek();
                break;
            default:
                cout << "* procesor " << my_rank << " >>>>>>>>>> unknown message in probeAndHandleMessages() <<<<<<<<< " << status.MPI_TAG << endl;
                throw 'x';
		}
		
		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
	}
}

void count()
{
	#ifdef LOG
	cout << "* procesor " << my_rank << " zahajil vypocet" << endl;
	#endif

	counter = 0;
    
    try {
    	while (1)
    	{
    		while (!s.empty())
    		{
    			if ((++counter % CHECK_MSG_AMOUNT) == 0)
    			{
                    probeAndHandleMessages();
    				counter = 0;
    			}

    			stack_item = s.front();
    			s.pop_front();

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
    						s.push_front(stack_item);
    						stack_item.x = 1;
    						s.push_front(stack_item);
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
    						s.push_front(stack_item);
    						stack_item.x = 0;
    						s.push_front(stack_item);
    					}
    				}
    			}
    		}

    		#ifdef LOG
    		cout << "* procesor " << my_rank << " nema praci" << endl;
    		#endif
		
    		if (!requestWork() || (p == 1)) break;
    	}
        cout << ">>>>>>>>>>>>> i'm idel " << my_rank << "<<<<<<<<<<<<<<<<<<" << endl;
    	if (p > 1) goToIdle();
    } catch (int i) {
        // proces obdrzel prikaz k ukonceni
    }
}

void writeResult()
{
	cout << "* reseni:" << endl << endl;
	cout << "incidencni tabulka (uzlu: " << nodes_total_count << ", hran: " 
		 << edges_total_count << "):" << endl;
	for (int i = 0; i < nodes_total_count; i++)
	{
		cout << i << " --> ";
		for (int j = 0; j < (int)incidence_table[i].size(); j++)
		{
			cout << incidence_table[i][j] << " ";
		}
		cout << endl;
	}
	cout << endl << "minimum uzlu: " << result.nodes_min_count << ", pocet reseni: " 
		 << (int)result.bit_arrays.size() << endl;
	for (int i = 0; i < (int)result.bit_arrays.size(); i++)
	{
		cout << i+1 << ": ";
		for (int j = 0; j < nodes_total_count; j++)
		{
			cout << result.bit_arrays[i][j];
		}
		cout << endl;
	}
}

void init()
{
	result.nodes_min_count = nodes_total_count;	
	
	stack_item.i = -1;
	stack_item.x = 1;
	stack_item.nodes_count = nodes_total_count;
	stack_item.bit_array = new int[nodes_total_count]; 
	for (int i = 0; i < nodes_total_count; i++) 
	{
		stack_item.bit_array[i] = 1;
	}

	if (p > 1)
	{
		idle = 0;
		sendIncidenceTable();
		v1.push_back(stack_item);
		countWork(v1, v2); // rozdeleni prace
		sendInitResult();
	}
	else 
	{
		s.push_back(stack_item);
	}
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

	request = new MPI_Request;
	
	if (my_rank == 0)
	{
		if (loadData("in.txt")) return 1;
		init();
		count();
		writeResult();
	}
	else
	{
		receiveIncidenceTable();
		receiveInitWork();
		receiveInitResult();
		count();
	}

	MPI_Finalize();
	
	return 0;
}