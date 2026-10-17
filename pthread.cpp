#include <iostream>
#include <pthread.h>
#include <cstring>
#include <queue>
using namespace std;

pthread_mutex_t mtx;
pthread_cond_t cv;
queue<int>q;
bool finished=false;

void* producer(void* x){
    for(int i=1;i<=100;i++){
        
        pthread_mutex_lock(&mtx);
        
        q.push(i);
        cout<<"任务: "<<i<<"进入队列"<<endl;

        pthread_cond_signal(&cv);
        pthread_mutex_unlock(&mtx);
    }
    pthread_mutex_lock(&mtx);
    finished = true;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&mtx);

    return nullptr;
}

void* work(void* x){
    int id=*(int*)x;
    while(true){
        {
            pthread_mutex_lock(&mtx);

            while(q.empty()&&!finished)pthread_cond_wait(&cv,&mtx);

            if(q.empty()&&finished){
                pthread_mutex_unlock(&mtx);
                return nullptr;
            }
            int x=q.front();
            q.pop();
            cout<<"任务: "<<x<<" 被工人: "<<id<<" 完成"<<endl;
            pthread_mutex_unlock(&mtx);
        }
    }
    return nullptr;
}

int main(){
    pthread_cond_init(&cv,NULL);
    pthread_mutex_init(&mtx,NULL);
    pthread_t pro,wor1,wor2,wor3;
    pthread_create(&pro,nullptr,producer,nullptr);

    int id1=1,id2=2,id3=3;
    pthread_create(&wor1,nullptr,work,&id1);
    pthread_create(&wor2,nullptr,work,&id2);
    pthread_create(&wor3,nullptr,work,&id3);

    pthread_join(pro,nullptr);
    pthread_join(wor1,nullptr);
    pthread_join(wor2,nullptr);
    pthread_join(wor3,nullptr);
        
    pthread_cond_destroy(&cv);
    pthread_mutex_destroy(&mtx);

    cout << "所有任务处理完毕"<<endl;

}