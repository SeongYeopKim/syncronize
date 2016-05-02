#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>

#define TEXT_SZ 256     // 공유변수 배열 사이즈
#define THREAD_NUMBER 2 // 스레드 갯수

void *timer(void*);
void *productAdd(void*);
int isStringNumber(char*);

struct shared_use_st {          //공유 변수 구조체
    int product_cnt;            //상품 카운트 등록이 되면 1 등록이 완료 되면 0으로 전환
    char some_text[TEXT_SZ];    //특정한 메시지를 보낼 때 사용, 현재 "end"를 통하여 경매가 종료되었음을 알린다.
    char productName[TEXT_SZ];  //상품의 이름을 받아올 변수
    char productPrice[TEXT_SZ]; //상품의 가격을 받아올 변수
};

void *shared_memory = (void *)0;        // 쉐어드 메모리 변수
struct shared_use_st *shared_stuff;     // 공유 변수 구조체 선언


int main()
{
    pthread_t p_thread[THREAD_NUMBER]; // 스레드 변수 선언 최대갯수만큼;
    
    int running = 1;    // 경매 작동
    
    int shmid;          // 공유메모리 return 값 저장
    int mykey = 600101; // 공유메모리 식별할 키값
    
    int thr_id; // 스레드 create return 값을 저장할 변수
    int data=1; // 스레드 파라미터 변수
    
    srand((unsigned int)getpid());  // 고정 된 랜덤값을 피하기 위해 pid를 활용하여 srand사용
    
    shmid = shmget((key_t)mykey, sizeof(struct shared_use_st), 0666 | IPC_CREAT);
    
    thr_id = pthread_create(&p_thread[0], NULL, timer, (void *)&data); // 타이머 스레드
    if (thr_id < 0){ //오류 확인용
        perror("thread create error : ");
        exit(0);
    }
    thr_id = pthread_create(&p_thread[1], NULL, productAdd, (void *)&data); // 상품 추가 스레드
    if (thr_id < 0){ // 오류 확인용
        perror("thread create error : ");
        exit(0);
    }
    
    if (shmid == -1) { // 오류 확인
        fprintf(stderr, "shmget failed\n");
        exit(EXIT_FAILURE);
    }
    
    shared_memory = shmat(shmid, (void *)0, 0); // 공유 메모리 생성하여 메모리로 사용
    if (shared_memory == (void *)-1) {          // 오류 확인
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    
    printf("경매시스템을 시작합니다.\n");
    printf("경매장을 오픈하였습니다.\n");
    
    shared_stuff = (struct shared_use_st *)shared_memory; // 구조체 변수를 공유메모리로
    while(running) { // 경매중인동안 돌아감
        if (strncmp(shared_stuff->some_text, "end", 3) == 0) {  // 도중에 end 메시지가 들어오면 경매 종료
            printf("경매를 종료합니다.");
            running = 0; // 경매가 종료
        }
    }
    
    if (shmdt(shared_memory) == -1) {  // 공유 메모리를 프로세스에서 분리 및 실패시 예외처리
        fprintf(stderr, "shmdt failed\n");
        exit(EXIT_FAILURE);
    }
    
    if (shmctl(shmid, IPC_RMID, 0) == -1) { // 공유 메모리 변경 또는 제거 , 여기서는 IPC_RMID로 제거한다. 예외처리 포함
        fprintf(stderr, "shmctl(IPC_RMID) failed\n");
        exit(EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
}

void *productAdd(void *data){   // 상품 등록 스레드
    char productName[30];   //상품 이름 변수
    char productPrice[30];  //상품 가격 변수
    int addMenu;            //상품 등록 메뉴 번호 변수
    
    sleep(3);
    while(1){
        printf("상품등록을 하시겠습니까? 1.등록 \n");
        printf(">");
        scanf("%d", &addMenu);
        
        if(addMenu==1){             // 등록을 선택했으면
            printf("상품이름 : ");
            scanf("%s", productName);
            printf("시작가격 : ");
            scanf("%s", productPrice);
            
            if(!isStringNumber(productPrice)){  // 숫자가 아니면 다시 등록
                printf("숫자만 입력 가능합니다. 다시 등록해주세요.\n");
                continue;
            }
            
            printf("\n등록 되었습니다.\n", productName,productPrice);
            shared_stuff->product_cnt=1;                        // 상품 등록 된 상태로 변경
            strcpy(shared_stuff->productName, productName);     // 이름과
            strcpy(shared_stuff->productPrice, productPrice);   // 가격을 넘긴다.
        }else{
            printf("등록은 1번 입니다. 다시 입력해주세요. \n");
        }
    }
}


void *timer(void *data){ // 타이머 스레드
    int time = 0;
    while(1){
        sleep(rand()%2);
        if(time++>50){
            strcpy(shared_stuff->some_text, "end");
        }
    }
}


int isStringNumber(char *s) {  //숫자인지 문자인지 판별하는 함수
    size_t size = strlen(s);
    if (size == 0) return 0; // 0바이트 문자열 제외
    
    for (int i = 0; i < (int) size; i++){
        if (s[i] == '.' || s[i] == '-' || s[i] == '+') continue;
        if (s[i] < '0' || s[i] > '9') return 0; // 알파벳 여부 확인
    }
    
    return 1; // return 1 (숫자)
}