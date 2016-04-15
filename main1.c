#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include<malloc.h>
#include<string.h>
#include<time.h>
#include"hash2.c"


int cid[20000];
float pcid[20000];
int right_cid=0;
int R_cid,c_count=0;
float PMAX=0;
float pmax=0,sum;

void get_data1(char* file){
		FILE* fp;
		char string[10000];
		if((fp=fopen(file,"r"))==NULL){
			printf("can not open it\n");
			exit(0);
		}
		char* k;
		char* v;
		while(!feof(fp)){
				fgets(string,sizeof(string),fp);
				if(!feof(fp)){
					k=strtok(string,"\t");
					v=strtok(NULL,"\t");
					cid[c_count]=atoi(k);
					pcid[c_count]=atof(v);
					c_count+=1;
				}
			}
			fclose(fp);
}

 struct thread{
	int cid;
	float pcid;
	char *word_list[100];
	Listnode *p;
	int status;
	int uniq_id;
	int wordcount;
	int threadnum;
	pthread_mutex_t src_mutex;
	pthread_mutex_t calc_mutex;
	pthread_cond_t calc_cond;
	pthread_mutex_t update_mutex;
	pthread_cond_t update_cond;
};

/////////////////////////////////////////////////////////////////
void * pthread(void * dat) {
	struct thread *arg = (struct thread *)dat;
	int id = 0;
	//
	while(arg->status > 0) {
		//做一些计算工作
		while(arg->uniq_id<arg->wordcount) {
			pthread_mutex_lock(&(arg->src_mutex));
			id = arg->uniq_id;
			sum=arg->pcid;
			arg->uniq_id += 1;
			pthread_mutex_unlock(&(arg->src_mutex));
			Listnode *p1=hash_table1_lookup(arg->word_list[id]);
			while(p1!=NULL){
				if(p1->Cid==arg->cid){
					sum=sum+(p1->Value);
					break;
				}
				else
					p1=p1->next;
			}
           if(sum<pmax){
			   pmax=sum;
			   R_cid=cid[id];
		   }
		}
		//printf("worker: %zu, work finish\n", pthread_self());
		//计算完成，发送更新信号
		pthread_mutex_lock(&(arg->update_mutex));
		arg->threadnum -=1;
		pthread_cond_signal(&(arg->update_cond));
		pthread_mutex_lock(&(arg->calc_mutex));//这一行为什么一定要放在这里？而不是放在解锁之后
		pthread_mutex_unlock(&(arg->update_mutex));
		//等待计算信号
		//pthread_mutex_lock(&(share_data2->calc_mutex));
		//printf("worker: %zu, wait to calc\n", pthread_self());
		pthread_cond_wait(&(arg->calc_cond), &(arg->calc_mutex));//为什么所有的计算线程都在等待同一个信号？这样做会不会有问题？
		pthread_mutex_unlock(&(arg->calc_mutex));
		//printf("worker: %zu, ready to run\n", pthread_self());
	}
	return 0;
}

int main(){
	clock_t  start,finish;
	start=clock();

	FILE *fp;
	char arr[1024];
	char file[20];
	printf("输入测试文件\n");
	scanf("%s",file);
	////
	if((fp=fopen(file,"r"))==NULL){
		printf("error");
		exit(0);
	}
	//////////////
	//
	//
	hash_table1_init();
	keep_data2("script2");
	get_data1("script1");

	char *title;
	char *word[100];
	float count=0,Rcount=0;
	while(!feof(fp)){
		fgets(arr,1024,fp);
		if(!feof(fp)){
			char *ch=strtok(arr,"\n");
			char *buf=strtok(arr,"\t");
			int matchcid;
			matchcid=atoi(buf);
			title=strtok(NULL,"\t");
			printf("%s 真正的类目：%d",title,matchcid);
			count++;
			int i=0;
			char *buffer=strtok(title," ");
			while(buffer!=NULL){
				word[i++]=buffer;
				buffer=strtok(NULL," ");
			}
			int wordcount=i;
			//////////////////////////
			////////////////////////////

	struct thread arg;
	//
	

	pthread_mutex_init(&arg.src_mutex, NULL);
	pthread_mutex_init(&arg.calc_mutex, NULL);
	pthread_cond_init(&arg.calc_cond, NULL);
	pthread_mutex_init(&arg.update_mutex, NULL);
	pthread_cond_init(&arg.update_cond, NULL);
	//
	//
	arg.wordcount=wordcount;
	int j;
	for(j=0;j<wordcount;j++){
		arg.word_list[j]=word[j];
	}
	arg.uniq_id=0;
	arg.threadnum=c_count;
	arg.status = 1; //1为进行中；0为任务完成


	pthread_t * pid_list=0;
	int k=0;
	arg.cid=cid[k];
	arg.pcid=pcid[k];
	k=k+1;

	if((pid_list=(pthread_t *)malloc(sizeof(pthread_t)*c_count))==NULL){
		printf("pid_list malloc error: %zu",sizeof(pthread_t)*c_count);
		exit(1);
	}

	//printf("in thread ready\n");
	//主线程就绪，启动工作线程
	
	for(i=0;i<c_count;i++){
		pthread_create(&(pid_list[i]), NULL, pthread, &arg);
	}

	//主线程进入工作状态
	while(arg.status > 0) {
		pthread_mutex_lock(&(arg.update_mutex));
		while(arg.threadnum > 0) {
			pthread_cond_wait(&(arg.update_cond), &(arg.update_mutex));
			//share_data2.count -= 1;//为什么不能在这里修改count，而是在工作线程里面修改？
			//printf("main worker, count: %d\n", arg.threadnum);//////////////可以关注一下这一行的输出结果
		}
		//printf("main wake\n");
		pthread_mutex_unlock(&(arg.update_mutex));
	
		//更新数据
		if(k>=c_count)
			arg.status=0;
		else{
			arg.cid=cid[k];
            arg.pcid=pcid[k];
			k=k+1;
			arg.threadnum=c_count;
			arg.uniq_id=0;
		}
		//printf("main worker, value: %d, base: %d\n", share_data2.value, share_data2.base);
		//
		pthread_mutex_lock(&(arg.calc_mutex));//为什么这里一定要拿到计算锁，去掉会怎么样？主线程和工作线程都在抢计算锁，会不会有问题？
		//printf("main worker get calc_mutex\n");
		//唤醒所有计算线程
		pthread_cond_broadcast(&(arg.calc_cond));
		pthread_mutex_unlock(&(arg.calc_mutex));
	}
	//printf("main worker, done\n");
	//等待工作线程退出
	for(i=0;i<c_count;i++){
		pthread_join(pid_list[i], NULL);
	}
	//释放资源
	pthread_mutex_destroy(&(arg.src_mutex));
	pthread_mutex_destroy(&(arg.calc_mutex));
	pthread_cond_destroy(&(arg.calc_cond));
	pthread_mutex_destroy(&(arg.update_mutex));
	pthread_cond_destroy(&(arg.update_cond));

	free(pid_list);

	printf("预测的类目： %d\n",R_cid);
	if(R_cid==matchcid)
		Rcount++;
	//
	//
}
}
float R=(Rcount/count)*100;
printf("正确率：%f%\n",R);
finish=clock();
double duration=finish-start;
printf("%f seconds\n",duration/CLOCKS_PER_SEC);
return 0;
}
///////////////////////////////////////////////////////////////////////////
/*int main(int argc, char * argv[]) {
	int dat_size = 0;
	int thread_num = 0;
	//
	if(argc < 3) {
		printf("demo: %s <data_size> <thread_num>\n", argv[0]);
		return 0;
	}
	//
	dat_size = atoi(argv[1]);
	thread_num = atoi(argv[2]);
	//
	test2(2, 1);
	//
	return 0;
}
*/
//编译方法：
//gcc -O2 -g -o mt multi_thread.c -lpthread -Wall
//运行：
//./mt 10 3
