#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <fcntl.h>

#define TEXT_SZ 256      // 공유변수 배열 사이즈
#define PRODUCT_SIZE 50  //상품 최대 갯수
#define THREAD_NUMBER 6 //스레드 갯수

void *enter(void*);
void *tender(int);
void *productAdd(void*);

struct shared_use_st {          //공유 변수 구조체
    int product_cnt;            //상품 카운트 등록이 되면 1 등록이 완료 되면 0으로 전환
    char some_text[TEXT_SZ];    //특정한 메시지를 보낼 때 사용, 현재 "end"를 통하여 경매가 종료되었음을 알린다.
    char productName[TEXT_SZ];  //상품의 이름을 받아올 변수
    char productPrice[TEXT_SZ]; //상품의 가격을 받아올 변수
};

struct product{         //상품 구조체
    int number;         //상품번호
    char name[30];      //상품이름
    int price;          //상품가격
    int nowPrice;       //상품현재가
    int nowPerson;      //상품 최고 입찰자
};

int person=0;                           // 입장한 사람의 수
int running;                            // 경매 진행중인지 아닌지
int product_total_size=0;               // 상품의 현재 갯수 (사이즈)
char buffer[TEXT_SZ];                   // 버퍼..
void *shared_memory = (void *)0;        // 쉐어드 메모리 변수
struct shared_use_st *shared_stuff;     // 공유 변수 구조체 선언
struct product pro[PRODUCT_SIZE];       // 상품 구조체 선언

pthread_mutex_t mux;                    // 뮤텍스 변수 선언

int main(){
    
    pthread_t p_thread[THREAD_NUMBER];  // 스레드 변수 선언 최대갯수만큼.
    int thr_id;     // 스레드 create return 값을 저장할 변수
    int data=1;     // 스레드 파라미터 변수
    
    running = 1;    // 경매 작동
    
    int shmid;          // 공유메모리 return 값 저장
    int mykey = 600101; // 공유메모리 식별할 키값
    int status;         // 상태
    
    pthread_attr_t attr;    // 뮤텍스 관련 attribute
    srand((unsigned int)getpid());  //입장하는 순서를 랜덤으로 한다.
    
    pthread_mutex_init(&mux, NULL); // 뮤텍스 초기화
    pthread_attr_init(&attr);       // attribute 초기화
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // JOINABLE 자식스레드가 킬되기를 기다린다.
    
    for(int i=0;i<5;i++){      // 스레드 생성 0~4
        thr_id = pthread_create(&p_thread[i], NULL, enter, (void *)&data); // 입장 스레드
        if (thr_id < 0){ // 오류 확인용
            perror("thread create error : ");
            exit(0);
        }
    }
    pthread_attr_destroy(&attr);
    
    for(int i=0;i<5;i++){   // 0~4까지의 스레드를 join 상태로
        pthread_join(&p_thread[i], (void**)&status);
    }
    
    thr_id = pthread_create(&p_thread[THREAD_NUMBER-1], NULL, productAdd, (void *)&data);
    // 상품 추가 대기 스레드,
    
    if (thr_id < 0){    // 오류 확인용
        perror("thread create error : ");
        exit(0);
    }
    
    shmid = shmget((key_t)mykey, sizeof(struct shared_use_st), 0666 | IPC_CREAT); // 공유 메모리 얻어오는 함수
    
    if (shmid == -1) {      // 오류 확인
        fprintf(stderr, "shmget failed\n");
        exit(EXIT_FAILURE);
    }
    
    shared_memory = shmat(shmid, (void *)0, 0);  // 공유 메모리 생성하여 메모리로 사용
    
    if (shared_memory == (void *)-1) {  // 오류 확인
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    
    printf("경매 시뮬레이션을 시작합니다.");
    
    shared_stuff = (struct shared_use_st *)shared_memory; // 구조체 변수를 공유메모리로
    
    shared_stuff->product_cnt=0;    // 상품 초기화
    strcpy(shared_stuff->some_text, ""); // 특정 메시지 초기화
    while(running) { //경매 진행하는 동안
        sleep(1);
    }
    
    if (shmdt(shared_memory) == -1) {   // 공유 메모리를 프로세스에서 분리 및 실패시 예외처리
        fprintf(stderr, "shmdt failed\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_destroy(&mux);  // 뮤텍스 파괴
    exit(EXIT_SUCCESS);           // 정상 종료
    
}

void *enter(void *data){    // 입장 스레드
    
    sleep(rand()%10+3); // 랜덤한 시간에 손님이 입장하는 시뮬레이션
    person++;           // 입장시 한명 증가
    
    printf("\n %d번째 손님이 입장하였습니다.", person);
    
    while(1){
        tender(person); //입찰 시작
    }
}

void *productAdd(void *data){
    sleep(3);  // 메모리 초기화로 인한 대기시간
    while(1){
        if(shared_stuff->product_cnt==1){   // 상품이 등록되었으면
            strcpy(pro[product_total_size].name, shared_stuff->productName);  // 상품의 이름
            pro[product_total_size].price = atoi(shared_stuff->productPrice);  // 가격
            pro[product_total_size].nowPrice = pro[product_total_size].price;  // 현재 가격 등록
            
            printf("\n등록이름 %s\n" ,pro[product_total_size].name);
            printf("등록가격 %d\n" ,pro[product_total_size].price);
            printf("현재 등록 된 상품의 수는 %d개 입니다.\n", product_total_size+1);
            
            // ------- 등록내용 출력
            
            product_total_size++;   // 상품 갯수 1개 증가
            shared_stuff->product_cnt--;    // 정상 등록하여 cnt 감소
        }
        
        if (strncmp(shared_stuff->some_text, "end", 3) == 0) { // 도중에 end 메시지가 들어오면 경매 종료
            printf("경매를 종료합니다.");
            running=0;  // 경매가 종료되며 메인스레드도 빠져나간다.
            
            for(int i=0;i<product_total_size;i++){ // 경매가 종료되며 입찰자 출력
                printf("\n%d번째 상품 낙찰자는 %d번째 손님입니다. 축하드립니다.\n", i+1, pro[i].nowPerson
                       );
            }
            
            return 0;
        }
        
    }
}

void *tender(int data){
    while(product_total_size==0){
        sleep(3);
    }
    
    while(1){
        if(running==1){
            sleep(rand()%10+3);
            pthread_mutex_lock(&mux);
            int proNum=rand()%product_total_size;
            int proPrice=rand()%100*100+pro[proNum].nowPrice;
            printf("\n %d번 고객, %d번 상품에 %d원 입찰\n", data, proNum+1, proPrice);
            pro[proNum].nowPrice = proPrice;
            pro[proNum].nowPerson = data;
            
            //            shared_stuff->written_by_you = 1;
            
            pthread_mutex_unlock(&mux);
        }
        
    }
}