#include <bits/stdc++.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

condition_variable cv;

bool finished = false;

// 已完成任务数量
atomic<int> completed = 0;

mutex mtx;
queue<int>q;
condition_variable notfull;
void producer(){
    for(int i=1;i<=100;i++){
        {
            unique_lock<mutex>lk(mtx);
            notfull.wait(lk,[]{
                return q.size()<10;
            });
            cout<<"任务: "<<i<<"进入队列"<<endl;
            q.push(i);
            lk.unlock();
        }
        cv.notify_one();
    }
    {
        lock_guard<mutex>lk(mtx);
        finished=true;
    }
    cv.notify_all();
}

void worker(int id){
    while(true){
        {
            unique_lock<mutex>lk(mtx);
            cv.wait(lk,[]{
                return !q.empty() || finished;
            });
            if(q.empty()&&finished)return;
            int x=q.front();
            q.pop();
            cout<<"任务: "<<x<<"被员工: "<<id<<"完成"<<endl;
            completed++;
        }
        notfull.notify_one();
    }
}

int main(){
    auto pro=thread(producer);
    auto wor1=thread(worker,1);
    auto wor2=thread(worker,2);
    auto wor3=thread(worker,3);
    pro.join();
    wor1.join();
    wor2.join();
    wor3.join();
    
    cout<<"共完成: "<<completed<<"个任务"<<endl;
    cout<<"线程结束"<<endl;
     
}

