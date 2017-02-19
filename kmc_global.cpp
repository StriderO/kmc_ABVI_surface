#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdio>
#include <vector>
#include "kmc_global.h"

using namespace std;

////////// GLOBAL VARIABLES //////////
long long int timestep;
double totaltime;

#define MAX_NNBR 20
int n1nbr, n2nbr;	// number of neighbors
int v1nbr[MAX_NNBR][3];	// indexes vectors of 1st neighbors
int v2nbr[MAX_NNBR][3];	// indexes vectors of 2nd neighbors
double vbra[3][3];	// coordinate vectors of bravice lattice

int nA, nB, nV, nAA, nBB, nAB, nM;
int mag; // sum of magnitization; should be conserved
int  states[nx][ny][nz];
bool  itlAB[nx][ny][nz]= {false};
bool    srf[nx][ny][nz]= {false};
bool marker[nx][ny][nz]= {false}; // mark V or M recb with I that calculated (inside ircal) and atoms that near a V and an I (from ircal to vrcal)

FILE * his_sol;		// history file of solute atoms
FILE * his_def;		// history file of defects
FILE * his_srf;		// history file of surface atoms
FILE * out_engy;	// out file of energy calculations
FILE * out_vdep;
FILE * out_eSGC;    // out file of semi-canonical ensemble e
FILE * out_sro;
FILE * out_log;

double h0;
double c1_44, c1_43, c1_42, c1_41, c1_33, c1_32, c1_31, c1_22, c1_21, c1_11, c1_40, c1_30, c1_20, c1_10, c1_00;
double c1_0_ABA, c1_0_ABB, c1_0_A, c1_0_B, c1_0_V, c1_0_AA, c1_0_AB, c1_0_BB;
double c2_44, c2_43, c2_42, c2_41, c2_33, c2_32, c2_31, c2_22, c2_21, c2_11, c2_40, c2_30, c2_20, c2_10, c2_00;
double c2_0_ABA, c2_0_ABB, c2_0_A, c2_0_B, c2_0_V, c2_0_AA, c2_0_AB, c2_0_BB;
double c1_MM, c1_MA, c1_MV, c1_MB;
double c2_MM, c2_MA, c2_MV, c2_MB;
double unc1_44, unc1_43, unc1_42, unc1_41, unc1_33, unc1_32, unc1_31, unc1_22, unc1_21, unc1_11, unc1_40, unc1_30, unc1_20, unc1_10, unc1_00;
double unc2_44, unc2_43, unc2_42, unc2_41, unc2_33, unc2_32, unc2_31, unc2_22, unc2_21, unc2_11, unc2_40, unc2_30, unc2_20, unc2_10, unc2_00;
bool is_e2nbr;

////////// GLOBAL FUNCTIONS //////////
void error(int nexit, string errinfo, int nnum, double num1, double num2){
	// exit number represents:
	// 0: main; 1: class_system; 2: class_events
	
	cout << "\nError: ";
	switch(nexit){
		case 0:  cout << "In main function "; break;
		case 1:  cout << "In class_system ";  break;
		case 2:  cout << "In class_events ";  break;
		default: cout << "UNKNOWN NEXIT" << endl;
	}

	cout << errinfo;
	switch(nnum){
		case 0:  cout << endl;				      break;
		case 1:  cout << ": " << num1 << endl;		      break;
		case 2:  cout << ": " << num1 << " " << num2 << endl; break;
		default: cout << "!!!ERROR FUNCTION MALFUNCTION!!! WRONG NNUM!!!" << endl;
	}
	cout << endl;
	exit(1); 
}

void error(int nexit, string errinfo, char c[]){
	// exit number represents:
	// 0: main; 1: class_system; 2: class_events
	
	cout << "\nError: ";
	switch(nexit){
		case 0:  cout << "In main function "; break;
		case 1:  cout << "In class_system ";  break;
		case 2:  cout << "In class_events ";  break;
		default: cout << "UNKNOWN NEXIT" << endl;
	}

	cout << errinfo << " " << c << endl;
	exit(1); 
}

double ran_generator(){
	static bool first= true;
	if(first){
		srand(time(NULL));
		first= false;
	}
	
	return rand()/((double) RAND_MAX+1.0);
}

int pbc(int x_, int nx_){ // Periodic Boundary Condition
	if	(x_<-nx_ || x_>=2*nx_)
		error(1, "(pbc) input x is out of bound", 2, x_, nx_);

	if	(x_<0) 		return (x_ + nx_);
	else if	(x_<nx_)	return  x_;
	else			return (x_ - nx_);
}

void write_conf(bool isRESTART){
	ofstream of_xyz;
	ofstream of_ltcp;
	
	// determine the names of conf files
    if(isRESTART){
//		of_xyz.open("RESTART.xyz");
		of_ltcp.open("RESTART");
    }
    else if(0==timestep){
		of_xyz.open("t0.xyz");
		of_ltcp.open("t0.ltcp");
	}
	else{
		char name_xyz[40], name_ltcp[40];
		sprintf(name_xyz, "%lld", timestep);  strcat(name_xyz, ".xyz");
		sprintf(name_ltcp, "%lld", timestep); strcat(name_ltcp, ".ltcp");
		
		of_xyz.open(name_xyz);
		of_ltcp.open(name_ltcp);
	}

	if((! isRESTART) && (!of_xyz.is_open())) error(1, "(write_conf) xyz file is not opened!");		// check
	if(!of_ltcp.is_open()) error(1, "(write_conf) ltcp file is not opened!");	// check
	
	// write out data
    if(isRESTART){
	    of_ltcp << nx*ny*nz-nA << "\n" << "ltcp " << timestep << " ";
	    of_ltcp << setprecision(15) << totaltime << "\n";
    }
    else{
	    of_xyz << nx*ny*nz << "\n" << "xyz " << timestep << " ";
	    of_xyz << setprecision(15) << totaltime << "\n";
	    of_ltcp << nx*ny*nz << "\n" << "ltcp " << timestep << " ";
	    of_ltcp << setprecision(15) << totaltime << "\n";
    }
	for(int i=0; i<nx; i ++){
		for(int j=0; j<ny; j ++){
			for(int k=0; k<nz; k ++){
				double x= i*vbra[0][0] + j*vbra[1][0] + k*vbra[2][0];
				double y= i*vbra[0][1] + j*vbra[1][1] + k*vbra[2][1];
				double z= i*vbra[0][2] + j*vbra[1][2] + k*vbra[2][2];
			
                if( 1==states[i][j][k] && (! isRESTART)){
					of_xyz  << states[i][j][k] << " " << x << " " << y << " " << z << endl;
					of_ltcp << states[i][j][k] << " " << i << " " << j << " " << k << " ";
                    of_ltcp << "0" << endl; // is_srf
                }
				if(-1==states[i][j][k]){
					if(! isRESTART) of_xyz  << states[i][j][k] << " " << x << " " << y << " " << z << endl;
					of_ltcp << states[i][j][k] << " " << i << " " << j << " " << k << " ";
                    of_ltcp << "0" << endl; // is_srf
                }
				else if (0==states[i][j][k] && (! itlAB[i][j][k])){
					if(! isRESTART) of_xyz  << states[i][j][k] << " " << x << " " << y << " " << z << endl;

					of_ltcp << states[i][j][k] << " " << i << " " << j << " " << k << " " << "0 0 0" << endl;
				}
	}}}
	
	of_xyz.close();
	of_ltcp.close();
}

void write_metrohis(){
	int nsol= 0;
	int nvcc= 0;
    
    fprintf(his_sol, "%d\n", nB);
	fprintf(his_sol, "T: %lld %e\n", timestep, totaltime);
    fprintf(his_def, "%d\n", nV);
	fprintf(his_def, "T: %lld %e\n", timestep, totaltime);
	for(int i=0; i<nx*ny*nz; i++){
		if( -1== *(&states[0][0][0]+i) ){
			nsol ++;
			fprintf(his_sol, "%d\n", i);
		}
		if(  0== *(&states[0][0][0]+i) ){
            nvcc ++;
		    fprintf(his_def, "0 %d 0 0 0\n", i);
        }
	}
    
    fflush(his_sol);
    fflush(his_def);
    
    if(nsol != nB) error(2, "(write_metrohis) sol value inconsistent", 2, nsol, nB);
    if(nvcc != nV) error(2, "(write_metrohis) vcc value inconsistent", 2, nvcc, nV);
}
