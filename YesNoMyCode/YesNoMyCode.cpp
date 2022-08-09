#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h> 
#include <conio.h>
#include "config.h"

using namespace std;

short samples[600000], MaxAmp=0;
float ThresholdZCR, DCshift=0;
double TotalEnergy=0, ThresholdEnergy, NoiseEnergy=0, TotalZCR=0; //TotalZCR is only for first few InitFrames
long start=0, stop=0, framecount, samplecount=0; //start and end marker for speech signal YES or NO
string line, filename;

ifstream InpSpeech;
ofstream ScaledInpSpeech,out;

int main()
{
		long i,j;
	//Try to open the file
		cout << "Enter the file name:  ";
    	getline(cin, filename);
    	InpSpeech.open(filename.c_str(), ios::in);
		if (!InpSpeech)
        {
    		cout << "\n**File failed to open**\n\n";
            InpSpeech.clear();
        }

		out.open("out.txt");
	//count the number of samples and frames in the file
		if (InpSpeech.is_open())
		{
			while ( !InpSpeech.eof() )
			{ 
				getline (InpSpeech,line);
				samplecount+=1;     
				if(samplecount > IgnoreSamples + 4) //first 4 lines in text file indiactes type of encoding of the speech
				{
					samples[samplecount - (IgnoreSamples + 5)] = (short)atoi(line.c_str()); //5=4+1 4-->indicates encoding, 1-->array index starts with 0
					DCshift+=samples[samplecount - (IgnoreSamples + 5)];
					if(abs(samples[samplecount - (IgnoreSamples + 5)])>MaxAmp)
						MaxAmp=abs(samples[samplecount - (IgnoreSamples + 5)]);
				}
			}
			InpSpeech.close();
		}
		samplecount = samplecount - (IgnoreSamples + 4);
		framecount = samplecount/framesize;
		DCshift=DCshift/samplecount;
		out << "Number of Samples = " << samplecount << "\n";
		out << "Number of frames = " << framecount << "\n";
		out << "DC shift needed is " << DCshift << "\n";
		out << "Maximum Amplitude = " << MaxAmp << "\n";

	//Store scaled samples in different file
		ScaledInpSpeech.open("ScaledInpSpeech.txt");
		for(i=0;i<samplecount;i++)
			ScaledInpSpeech << (samples[i] - DCshift)*Ampscale/MaxAmp << "\n";
		ScaledInpSpeech.close();
	//use the scaled samples
		InpSpeech.open("ScaledInpSpeech.txt", ios::in);
		if (!InpSpeech)
        {
    		cout << "\n**File failed to open**\n\n";
            InpSpeech.clear();
        }
		InpSpeech.close();
	//ZCR and Energy calculation
		float AvgZCR[400], AvgEnergy[400];
		for(i=0;i<framecount;i++)
		{
			AvgZCR[i]=0;
			AvgEnergy[i]=0;
			for(j=0;j<framesize-1;j++)
			{
				if((samples[i*framesize + j]-DCshift)*Ampscale/MaxAmp*((samples[i*framesize + j + 1]-DCshift)*Ampscale*1.0 < 0))
					AvgZCR[i]+=1;
				AvgEnergy[i]+=1.0*(samples[i*framesize + j]-DCshift)*Ampscale/MaxAmp*(samples[i*framesize + j]-DCshift)*Ampscale/MaxAmp;
			}
			//out << "ZCR in "<<i+1<<"th frame is "<< AvgZCR[i] << "\n";
			AvgZCR[i]/=framesize;
			//out << "Avergae ZCR in\t\t "<<i+1<<"th frame is "<< AvgZCR[i] << "\n";
			AvgEnergy[i]+=1.0*(samples[i*framesize + j]-DCshift)*Ampscale/MaxAmp*(samples[i*framesize + j]-DCshift)*Ampscale/MaxAmp;
			//out << "Energy in\t "<<i+1<<"th frame is "<< AvgEnergy[i] << "\n";
			AvgEnergy[i]/=framesize;
			//out << "Average Energy in \t\t\t"<<i+1<<"th frame is "<< AvgEnergy[i] << "\n";
			TotalEnergy+=AvgEnergy[i];
		}
	//calculate noise energy to decide threshold for energy
		for(i=0;i<InitFrames;i++)
		{
			TotalZCR+=AvgZCR[i];
			NoiseEnergy+=AvgEnergy[i];
		}
		ThresholdZCR=TotalZCR/InitFrames;
		NoiseEnergy/=InitFrames;
		ThresholdZCR*=0.9;
		//ThresholdEnergy=TotalEnergy/framecount; //threshold energy is the average of all averaged energies
		//ThresholdEnergy*=0.9;
		ThresholdEnergy=NoiseEnergy*10;
		bool flag=false;
	//start and end marker of speech
		for(i=0;i<framecount-3;i++)
		{
			//if(AvgEnergy[i+1]>ThresholdEnergy)
			if(AvgZCR[i+1]<ThresholdZCR || AvgEnergy[i+1]>ThresholdEnergy)
			{
				if(flag == false && AvgEnergy[i+2]>ThresholdEnergy && AvgEnergy[i+3]>ThresholdEnergy)
				{
					start = i  ; //i th frame is the starting frame marker
					out << "Starting frame is "<< start+1 <<"th frame and starting sample is "<< (start+1)*framesize <<"\n";
					out << "Starting time = " << 1.0*(start + 1)*framesize/samplingrate << " seconds\n" ;
					flag = true;
				}
			}
			else if(flag == true && AvgZCR[i] > ThresholdZCR && AvgEnergy[i] < ThresholdEnergy && AvgEnergy[i-1] < ThresholdEnergy && AvgEnergy[i-2] < ThresholdEnergy)
			{
					stop = i ;
					out << "Ending frame is "<< stop+1 <<"th frame and Ending sample is "<< (stop+1)*framesize<<"\n";
					out << "Ending time = " << 1.0*(stop + 1)*framesize/samplingrate << " seconds\n" ;
					flag = false;
			}
		}
	
		double testavgE=0, testavgZ=0;
		for(i=start;i<=stop;i++)
		{
			testavgE+=AvgEnergy[i];
			testavgZ+=AvgZCR[i];
		}
		testavgE/=(stop-start+1);
		testavgZ/=(stop-start+1);
		out << "Average Energy of the speech signal is "<< testavgE << "\n";
		out << "Average ZCR of the speech signal is "<< testavgZ*framesize << " \n";
		
		float lcount=0, rcount=0;
		for(i=stop;i>=start;i--)
		{
			if(AvgZCR[i]>testavgZ && AvgEnergy[i] < testavgE)
				rcount++;
			if(AvgZCR[start+stop-i]< testavgZ && AvgEnergy[i] < testavgE)
				lcount++;
		}

		//cout << "right count is "<<rcount << "\n";
		//cout << "left count is "<<lcount << "\n";

		//float lcount=0, rcount=0;
/*		bool tflag=false;
		for(i=start;i<=stop;i++)
			if(tflag==false && AvgEnergy[i] < testavg)
			{
				lcount++;
				if(AvgEnergy[i+1] > testavg)
					tflag=true;
			}
			else if(tflag==true&&AvgEnergy[i] < testavg)
				rcount++;
		cout << "lcount = "<<lcount << endl;
		cout << "rcount = "<<rcount << endl;
		*/
		long mark;
		mark=(stop+start)/2;
		if(mark == 0)
			out << "Non speech signal\n";
		//else if(rcount>3*lcount) //to work with mohit sir voice + huge noise
		else if(rcount > 10)
			out << "speech is YES signal\n";
		else
			out << "speech is NO signal\n\n";
		cout << "NOTE: To see the speech samples, check ScaledInpSpeech.txt file \n";
		out.close();
		cout << "Check the output in out.txt file\n";
	getch();
	return 0;
}