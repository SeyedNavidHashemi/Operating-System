#include <iostream>
#include <sndfile.h>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>
#include <random>
using namespace std;
using namespace chrono; 

const string output_file = "output";
const string type_exec = "_ser.wav";
const int delta_f = 2;
const int f0 = 50;
const int n = 2;
const int M = 100;
const int N = 100;

void readWavFile(const std::string& inputFile, std::vector<float>& data, SF_INFO& fileInfo) {
    SNDFILE* inFile = sf_open(inputFile.c_str(), SFM_READ, &fileInfo);
    if (!inFile) {
        std::cerr << "Error opening input file: " << sf_strerror(NULL) << std::endl;
        exit(1);
    }

    data.resize(fileInfo.frames * fileInfo.channels);
    sf_count_t numFrames = sf_readf_float(inFile, data.data(), fileInfo.frames);
    if (numFrames != fileInfo.frames) {
        std::cerr << "Error reading frames from file." << std::endl;
        sf_close(inFile);
        exit(1);
    }

    sf_close(inFile);
    // cout << "Successfully read " << numFrames << " frames from " << inputFile << std::endl;
}

void writeWavFile(const std::string& outputFile, const std::vector<float>& data,  SF_INFO& fileInfo) {
    sf_count_t originalFrames = fileInfo.frames;
    SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &fileInfo);
    if (!outFile) {
        std::cerr << "Error opening output file: " << sf_strerror(NULL) << std::endl;
        exit(1);
    }

    sf_count_t numFrames = sf_writef_float(outFile, data.data(), originalFrames);
    if (numFrames != originalFrames) {
        std::cerr << "Error writing frames to file." << std::endl;
        sf_close(outFile);
        exit(1);
    }

    sf_close(outFile);
    // cout << "Successfully wrote " << numFrames << " frames to " << outputFile << std::endl;
}

void apply_band_pass_filter(vector<float> audioData, vector<float>& audioData_BP)
{
    for(int i = 0; i < audioData.size(); i++)
    {
        audioData_BP[i] = pow(audioData[i], 2) / (pow(audioData[i], 2) + pow(delta_f, 2));
    }
}

void apply_notch_filter(vector<float> audioData, vector<float>& audioData_Notch)
{
    for(int i = 0; i < audioData.size(); i++)
    {
        float part_1 = audioData[i] / f0;
        float part_2 = 2*n;
        audioData_Notch[i] = 1 / (pow(part_1, part_2) + 1);
    }
}

vector<float> generate_random_coefficients(int num)
{
    vector<float> b;
    random_device rd;  
    mt19937 gen(rd()); 
    uniform_real_distribution<> dis(0.0, 1.0);

    for (int i = 0; i < num; i++) 
    {
        float random_number = dis(gen);
        b.push_back(random_number);
    }

    return b;
}

void apply_FIR_filter(vector<float> audioData, vector<float>& audioData_FIR)
{
    vector<float> h = generate_random_coefficients(M);

    for(int n = 0; n < audioData.size(); n++)
    {
        audioData_FIR[n] = 0;
        for(int i = 0; i < M; i++)
        {
            if (n - i >= 0)
                audioData_FIR[n] += (h[i] * audioData[n - i]);
        }

    }
}

void apply_IIR_filter(vector<float> audioData, vector<float>& audioData_IIR)
{
    vector<float> a = generate_random_coefficients(N);
    vector<float> b = generate_random_coefficients(M);
    for(int n = 0; n < audioData.size(); n++)
    {
        audioData_IIR[n] = 0;
        for(int j = 0; j < N; j++)
        {
            if (n - j >= 0)
                audioData_IIR[n] += (a[j] * audioData_IIR[n - j]);
        }
        for(int k = 0; k < M; k++)
        {
            if (n - k >= 0)
                audioData_IIR[n] += (b[k] * audioData_IIR[n - k]);
        }
    }
}

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        perror("Invalid Arguments");
        exit(0);
    }
    string inputFile = argv[1];

    SF_INFO fileInfo1, fileInfo2, fileInfo3, fileInfo4;
    vector<float> audioData;

    vector<float> audioData_BP, audioData_Notch, audioData_FIR, audioData_IIR;

    std::memset(&fileInfo1, 0, sizeof(fileInfo1));
    std::memset(&fileInfo1, 0, sizeof(fileInfo2));
    std::memset(&fileInfo1, 0, sizeof(fileInfo3));
    std::memset(&fileInfo1, 0, sizeof(fileInfo4));

    auto start_exec = high_resolution_clock::now();
//Bandpath filter----------------------------------------------------------------
    
    //read
    auto start_read = high_resolution_clock::now();
    readWavFile(inputFile, audioData, fileInfo1);
    auto end_read = high_resolution_clock::now();
    duration<double> read_time = end_read - start_read;
    cout << "Read: " << read_time.count()*1000 << " ms" << endl;

    //filter
    auto start_BP = high_resolution_clock::now();
    audioData_BP = audioData;
    apply_band_pass_filter(audioData, audioData_BP);
    auto end_BP = high_resolution_clock::now();
    duration<double> BP_time = end_BP - start_BP;
    cout << "Band-pass Filter: " << BP_time.count()*1000 << " ms" << endl;

    //write
    writeWavFile(output_file + "_BP" + type_exec, audioData_BP, fileInfo1);

//Notch filter--------------------------------------------------------------------

    //read
    readWavFile(inputFile, audioData, fileInfo2);

    //filter
    auto start_notch = high_resolution_clock::now();
    audioData_Notch = audioData;
    apply_notch_filter(audioData, audioData_Notch);
    auto end_notch = high_resolution_clock::now();
    duration<double> notch_time = end_notch - start_notch;
    cout << "Notch Filter: " << notch_time.count()*1000 << " ms" << endl;

    //write
    writeWavFile(output_file + "_Notch" + type_exec, audioData_Notch, fileInfo2);

//FIR filter-----------------------------------------------------------------------
    
    //read
    readWavFile(inputFile, audioData, fileInfo3);

    //filter
    auto start_FIR = high_resolution_clock::now();
    audioData_FIR = audioData;
    apply_FIR_filter(audioData, audioData_FIR);
    auto end_FIR = high_resolution_clock::now();
    duration<double> FIR_time = end_FIR - start_FIR;
    cout << "FIR Filter: " << FIR_time.count()*1000 << " ms" << endl;

    
    //write
    writeWavFile(output_file + "_FIR" + type_exec, audioData_BP, fileInfo3);

//IIR filter-----------------------------------------------------------------------

    //read
    readWavFile(inputFile, audioData, fileInfo4);

    //filter
    auto start_IIR = high_resolution_clock::now();
    audioData_IIR = audioData;
    apply_IIR_filter(audioData, audioData_IIR);
    auto end_IIR = high_resolution_clock::now();
    duration<double> IIR_time = end_IIR - start_IIR;
    cout << "IIR Filter: " << IIR_time.count()*1000 << " ms" << endl;

    
    //write
    writeWavFile(output_file + "_IIR" + type_exec, audioData_BP, fileInfo4);


    auto end_exec = high_resolution_clock::now();
    duration<double> complete_time = end_exec - start_exec;
    cout << "Execution: " << complete_time.count()*1000 << " ms" << endl;
 
    return 0;
}
