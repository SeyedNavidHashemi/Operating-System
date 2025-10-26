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
const string type_exec = "_par.wav";
const int NumOfThreads = 25;
const int delta_f = 2;
const int f0 = 50;
const int n = 2;
const int M = 100;
const int N = 100;

struct ThreadData {
    int thread_id;
    int start_idx;  
    int end_idx;      
    int extended_start_idx;  
    int extended_end_idx;    
    const vector<float>* input;
    vector<float>* output;
    const vector<float>* a;
    const vector<float>* h;
};

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

void* band_pass_filter_thread(void* arg) 
{
    ThreadData* data = (ThreadData*)arg;
    for (int i = data->start_idx; i < data->end_idx; i++) 
    {
        (*data->output)[i] = pow((*data->input)[i], 2) / (pow((*data->input)[i], 2) + pow(delta_f, 2));
    }
    pthread_exit(nullptr);
}

void apply_band_pass_filter_multithreaded(const vector<float>& input, vector<float>& output, int num_threads) 
{
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    int chunk_size = input.size() / num_threads;

    for (int i = 0; i < num_threads; i++) 
    {
        thread_data[i].thread_id = i;
        thread_data[i].start_idx = i * chunk_size;
        thread_data[i].end_idx = (i == num_threads - 1) ? input.size() : (i + 1) * chunk_size;
        thread_data[i].input = &input;
        thread_data[i].output = &output;
        pthread_create(&threads[i], nullptr, band_pass_filter_thread, &thread_data[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
    }
}

void* notch_filter_thread(void* arg) 
{
    ThreadData* data = (ThreadData*)arg;
    for (int i = data->start_idx; i < data->end_idx; i++) 
    {
        float part_1 = (*data->input)[i] / f0;
        float part_2 = 2*n;
        (*data->output)[i] = 1 / (pow(part_1, part_2) + 1);
    }
    pthread_exit(nullptr);
}

void apply_notch_filter_multithreaded(const vector<float>& input, vector<float>& output, int num_threads) 
{
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    int chunk_size = input.size() / num_threads;

    for (int i = 0; i < num_threads; i++) 
    {
        thread_data[i].thread_id = i;
        thread_data[i].start_idx = i * chunk_size;
        thread_data[i].end_idx = (i == num_threads - 1) ? input.size() : (i + 1) * chunk_size;
        thread_data[i].input = &input;
        thread_data[i].output = &output;
        pthread_create(&threads[i], nullptr, notch_filter_thread, &thread_data[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
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

void* FIR_filter_thread(void* arg) 
{
    ThreadData* data = (ThreadData*)arg;
    int start_idx = data->start_idx;
    int end_idx = data->end_idx;
    int extended_start_idx = data->extended_start_idx;
    int extended_end_idx = data->extended_end_idx;

    const vector<float>& input = *(data->input);
    vector<float>& output = *(data->output);
    const vector<float>& h = *(data->h);


    for (int n = extended_start_idx; n < extended_end_idx; n++)
    {
        float result = 0.0f;
        for (int i = 0; i < M; i++) 
        {
            if (n - i >= 0 && n - i < input.size()) 
            {
                result += h[i] * input[n - i];
            }
        }
        if (n >= start_idx && n < end_idx) 
        {
            output[n] = result;
        }
    }

    pthread_exit(NULL);
}

void apply_FIR_filter_multithreaded(const vector<float>& audioData, vector<float>& audioData_FIR, int num_threads) 
{
    pthread_t threads[num_threads];
    ThreadData threadData[num_threads];
    int chunk_ize = audioData.size() / num_threads;
    vector<float> h = generate_random_coefficients(N);

    for (int i = 0; i < num_threads; ++i) 
    {
        threadData[i].thread_id = i;
        threadData[i].start_idx = i * chunk_ize;
        threadData[i].end_idx = (i == num_threads - 1) ? audioData.size() : (i + 1) * chunk_ize;
        threadData[i].extended_start_idx = max(0, threadData[i].start_idx - (M - 1));
        threadData[i].extended_end_idx = threadData[i].end_idx;
        threadData[i].input = &audioData;
        threadData[i].output = &audioData_FIR;
        threadData[i].h = &h;

        pthread_create(&threads[i], NULL, FIR_filter_thread, (void*)&threadData[i]);
    }

    for (int i = 0; i < num_threads; ++i) 
    {
        pthread_join(threads[i], NULL);
    }
}

void* IIR_filter_thread(void* arg) 
{
    ThreadData* data = (ThreadData*)arg;
    int start_idx = data->start_idx;
    int end_idx = data->end_idx;
    int extended_start_idx = data->extended_start_idx;
    int extended_end_idx = data->extended_end_idx;

    const vector<float>& input = *(data->input);
    vector<float>& output = *(data->output);
    const vector<float>& a = *(data->a);


    for (int n = extended_start_idx; n < extended_end_idx; n++)
    {
        float result = 0;
        for(int j = 0; j < N; j++)
        {
            if (n - j >= 0)
                result += (a[j] * input[n - j]);
        }
        if (n >= start_idx && n < end_idx) 
        {
            output[n] = result;
        }
    }

    pthread_exit(NULL);
}

void apply_IIR_filter_multithreaded(const vector<float>& audioData, vector<float>& audioData_IIR, int num_threads)
{
    pthread_t threads[num_threads];
    ThreadData threadData[num_threads];
    int chunk_ize = audioData.size() / num_threads;
    vector<float> a = generate_random_coefficients(N);
    vector<float> b = generate_random_coefficients(M);

    for (int i = 0; i < num_threads; ++i) 
    {
        threadData[i].thread_id = i;
        threadData[i].start_idx = i * chunk_ize;
        threadData[i].end_idx = (i == num_threads - 1) ? audioData.size() : (i + 1) * chunk_ize;
        threadData[i].extended_start_idx = max(0, threadData[i].start_idx - (M - 1));
        threadData[i].extended_end_idx = threadData[i].end_idx;
        threadData[i].input = &audioData;
        threadData[i].output = &audioData_IIR;
        threadData[i].a = &a;

        pthread_create(&threads[i], NULL, IIR_filter_thread, (void*)&threadData[i]);
    }

    for (int i = 0; i < num_threads; ++i) 
    {
        pthread_join(threads[i], NULL);
    }

    for(int n = 0; n < audioData.size(); n++)
    {
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
    // pthread_mutex_init(&write_mutex, nullptr);

//Bandpath filter----------------------------------------------------------------
    //read
    auto start_read_1 = high_resolution_clock::now();
    readWavFile(inputFile, audioData, fileInfo1);
    auto end_read_1 = high_resolution_clock::now();
    audioData_BP.resize(audioData.size());
    duration<double> read_time_1 = end_read_1 - start_read_1;
    cout << "Read: " << read_time_1.count()*1000 << " ms" << endl;

    //filter
    int best_num_threads_BP = 1;
    double best_time_BP = numeric_limits<double>::max();
    for (int num_threads = 1; num_threads <= NumOfThreads; num_threads++) 
    { 
        auto start_BP = high_resolution_clock::now();
        apply_band_pass_filter_multithreaded(audioData, audioData_BP, num_threads);
        auto end_BP = high_resolution_clock::now();
        duration<double> time_BP = end_BP - start_BP;

        if (time_BP.count() < best_time_BP) 
        {
            best_time_BP = time_BP.count();
            best_num_threads_BP = num_threads;
        }
    }
    // auto start_BP = high_resolution_clock::now();
    // apply_band_pass_filter_multithreaded(audioData, audioData_BP, NumOfThreads);
    // auto end_BP = high_resolution_clock::now();
    // duration<double> time_BP = end_BP - start_BP;
    cout << "Band-pass Filter: " << best_time_BP * 1000 << " ms" << endl;
    //  (num_of_threads: " << best_num_threads_BP << ")" << endl;

    // Write
    auto start_write_1 = high_resolution_clock::now();
    writeWavFile(output_file + "_BP" + type_exec, audioData_BP, fileInfo1);
    auto end_write_1 = high_resolution_clock::now();
    duration<double> write_time_1 = end_write_1 - start_write_1;

//Notch filter----------------------------------------------------------------
    //read
    auto start_read_2 = high_resolution_clock::now();
    readWavFile(inputFile, audioData, fileInfo2);
    auto end_read_2 = high_resolution_clock::now();
    duration<double> read_time_2 = end_read_2 - start_read_2;
    audioData_Notch.resize(audioData.size());

    //filter
    int best_num_threads_Notch = 1;
    double best_time_Notch = numeric_limits<double>::max();
    for (int num_threads = 1; num_threads <= NumOfThreads; num_threads++) 
    { 
        auto start_Notch = high_resolution_clock::now();
        apply_notch_filter_multithreaded(audioData, audioData_Notch, num_threads);
        auto end_Notch = high_resolution_clock::now();
        duration<double> time_Notch = end_Notch - start_Notch;

        if (time_Notch.count() < best_time_Notch) 
        {
            best_time_Notch = time_Notch.count();
            best_num_threads_Notch = num_threads;
        }
    }
    // auto start_Notch = high_resolution_clock::now();
    // apply_notch_filter_multithreaded(audioData, audioData_Notch, NumOfThreads);
    // auto end_Notch = high_resolution_clock::now();
    // duration<double> time_Notch = end_Notch - start_Notch;
    cout << "Notch Filter: " << best_time_Notch * 1000 << " ms" << endl;
    //  (num_of_threads: " << best_num_threads_Notch << ")" << endl;

    // Write
    auto start_write_2 = high_resolution_clock::now();
    writeWavFile(output_file + "_Notch" + type_exec, audioData_Notch, fileInfo2);
    auto end_write_2 = high_resolution_clock::now();
    duration<double> write_time_2 = end_write_2 - start_write_2;

//FIR filter----------------------------------------------------------------
    //read
    auto start_read_3 = high_resolution_clock::now();
    readWavFile(inputFile, audioData, fileInfo3);
    auto end_read_3 = high_resolution_clock::now();
    duration<double> read_time_3 = end_read_3 - start_read_3;
    audioData_FIR.resize(audioData.size());

    //filter
    int best_num_threads_FIR = 1;
    double best_time_FIR = numeric_limits<double>::max();
    for (int num_threads = 1; num_threads <= NumOfThreads; num_threads++) 
    { 
        auto start_FIR = high_resolution_clock::now();
        apply_FIR_filter_multithreaded(audioData, audioData_FIR, num_threads);
        auto end_FIR = high_resolution_clock::now();
        duration<double> time_FIR = end_FIR - start_FIR;

        if (time_FIR.count() < best_time_FIR) 
        {
            best_time_FIR = time_FIR.count();
            best_num_threads_FIR = num_threads;
        }
    }
    // auto start_FIR = high_resolution_clock::now();
    // apply_FIR_filter_multithreaded(audioData, audioData_FIR, NumOfThreads);
    // auto end_FIR = high_resolution_clock::now();
    // duration<double> time_FIR = end_FIR - start_FIR;
    cout << "FIR Filter: " << best_time_FIR * 1000 << " ms" << endl;
    //  (num_of_threads: " << best_num_threads_FIR << ")" << endl;

    // Write
    auto start_write_3 = high_resolution_clock::now();
    writeWavFile(output_file + "_FIR" + type_exec, audioData_FIR, fileInfo3);
    auto end_write_3 = high_resolution_clock::now();
    duration<double> write_time_3 = end_write_3 - start_write_3;

//IIR filter----------------------------------------------------------------
//read
    auto start_read_4 = high_resolution_clock::now();
    readWavFile(inputFile, audioData, fileInfo4);
    auto end_read_4 = high_resolution_clock::now();
    duration<double> read_time_4 = end_read_4 - start_read_4;
    audioData_IIR.resize(audioData.size());

    //filter
    int best_num_threads_IIR = 1;
    double best_time_IIR = numeric_limits<double>::max();
    for (int num_threads = 1; num_threads <= NumOfThreads; num_threads++) 
    { 
        auto start_IIR = high_resolution_clock::now();
        apply_IIR_filter_multithreaded(audioData, audioData_IIR, num_threads);
        auto end_IIR = high_resolution_clock::now();
        duration<double> time_IIR = end_IIR - start_IIR;

        if (time_IIR.count() < best_time_IIR) 
        {
            best_time_IIR = time_IIR.count();
            best_num_threads_IIR = num_threads;
        }
    }
    // auto start_IIR = high_resolution_clock::now();
    // apply_IIR_filter_multithreaded(audioData, audioData_IIR, NumOfThreads);
    // auto end_IIR = high_resolution_clock::now();
    // duration<double> time_IIR = end_IIR - start_IIR;
    cout << "IIR Filter: " << best_time_IIR * 1000 << " ms" << endl;
    //  (num_of_threads: " << best_num_threads_IIR << ")" << endl;

    // Write
    auto start_write_4 = high_resolution_clock::now();
    writeWavFile(output_file + "_IIR" + type_exec, audioData_IIR, fileInfo4);
    auto end_write_4 = high_resolution_clock::now();
    duration<double> write_time_4 = end_write_4 - start_write_4;

    double complete_time = read_time_1.count() + read_time_2.count() + read_time_3.count() + read_time_4.count()
                         + write_time_1.count() + write_time_2.count() + write_time_3.count() + write_time_4.count()
                         + best_time_BP + best_time_FIR + best_time_IIR + best_time_Notch;

    cout << "Execution: " << complete_time * 1000 << " ms" << endl;
    
    return 0;
}
