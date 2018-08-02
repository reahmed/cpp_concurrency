#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <future>

// COMPILE with:
// g++ -std=c++14 -pthread <file>.cpp

double add(std::vector<double>::iterator it1, std::vector<double>::iterator it2)
{
  double sum = 0.;
  for (auto it = it1; it != it2; ++it)
    sum += *it;
  return sum;
}

void multithread_add(std::vector<double>& x, int N)
{
  auto t1 = std::chrono::high_resolution_clock::now();

  auto x0 = x.begin();
  auto sz = x.size();

  std::vector<std::future<double>> f;
  auto p = std::launch::async;
  for (auto i = 0; i < N; ++i)
  {    
    f.emplace_back(std::async(p, add, x.begin() + sz/N*i, x.begin() + sz/N*(i+1)));
  }

  double y = 0;
  for (auto i = 0; i < N; ++i)
    y += f[i].get();

  auto t2 = std::chrono::high_resolution_clock::now();
  auto t  = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
  std::cout << "\nNUM_THREADS: " << N << std::endl;
  std::cout << "Multi-thread runtime is " << t.count() << " us" << std::endl;
  std::cout << "Multi-thread result is "  << y         << std::endl; 
}

void add2(std::vector<double>::iterator it1, std::vector<double>::iterator it2, double& out)
{
  // DONT SET OUT DIRECTLY, MUCH SLOWER DUE TO ACCESS VIA POINTER!
  double sum = 0.;
  for (auto it = it1; it != it2; ++it)
    sum += *it;
  out = sum;
}

void multithread_add2(std::vector<double>& x, int N)
{

  auto t1 = std::chrono::high_resolution_clock::now();

  auto x0 = x.begin();
  auto sz = x.size();

  std::vector<double> res(N);
  std::vector<std::thread> ths;
  for (auto i = 0; i < N; ++i)
  {
    ths.emplace_back(std::thread(add2, x.begin() + sz/N*i, x.begin() + sz/N*(i+1), std::ref(res[i])));
  }

  for (auto i = 0; i < N; ++i)
  {
    ths[i].join();
  }

  double y = 0;
  for (auto i = 0; i < N; ++i)
    y += res[i];

  auto t2 = std::chrono::high_resolution_clock::now();
  auto t  = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
  std::cout << "\nNUM_THREADS: " << N << std::endl;
  std::cout << "Multi-thread runtime is " << t.count() << " us" << std::endl;
  std::cout << "Multi-thread result is "  << y         << std::endl; 
}

int main()
{
  std::vector<double> x(1 << 24); // ~ 16 million elements

  // for (auto& e : x)
  //  e = 1.0;


  // sum converges to pi^2 / 6
  for (auto i = 0; i < x.size(); ++i)
    x[i] = 1/double(i+1) * 1/double(i+1);
 
  auto t1 = std::chrono::high_resolution_clock::now();
  auto y  = add(x.begin(), x.end());
  auto t2 = std::chrono::high_resolution_clock::now();
  auto t  = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
  std::cout << "Single thread runtime is " << t.count() << " us" << std::endl;
  std::cout << "Single thread result is "  << y         << std::endl;  

  multithread_add2(x, 1);
  multithread_add2(x, 4);
  multithread_add2(x, 8);
  multithread_add2(x, 16);
  multithread_add2(x, 32);  

  multithread_add(x, 1);
  multithread_add(x, 4);
  multithread_add(x, 8);
  multithread_add(x, 16);
  multithread_add(x, 32);  
  
  return 0;
}
