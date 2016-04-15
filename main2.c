#include<stdio.h>
#include<string.h>
#include<malloc.h>
#include<stdlib.h>
#include"hash2.c"
#include<pthread.h>
#include<time.h>


pthread_mutex_t mut;
pthread_mutex_t mut1;

typedef struct{
	int sta;
	int end;
	int wordcount;
	char *word[50];
}ARG;

int cid[20000];
float pcid[20000];
int right_cid=0;
int R_cid,c_count=0;
float PMAX=0;
float pmax,sum,temp=0;

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

void* pthread(void* arg){
	ARG *a=(ARG*) arg;
	int sta=a->sta;
	int end=a->end;
	int wordcount=a->wordcount;
	int j,i;
	pthread_mutex_lock(&mut1);
	for(i=sta;i<=end;i++){
		//pthread_mutex_lock(&mut1);
		temp=0;
		//pthread_mutex_unlock(&mut1);
		for(j=0;j<wordcount;j++){
			Listnode *p2=hash_table1_lookup(a->word[j]);
			while(p2){
				if(p2->Cid==cid[i]){
					temp=temp+(p2->Value);
					break;
				}
				else
					p2=p2->next;
			}
		}
		if((temp+pcid[i])<pmax){
			pmax=temp+pcid[i];
			R_cid=cid[i];
		}
	}

	for(j=0;j<wordcount;j++)
		printf("%s,",a->word[j]);
	printf("\n");
	printf("%d-%d:线程%zu预测的类目：%d		%f\n",sta,end,pthread_self(),R_cid,pmax);
	pthread_mutex_unlock(&mut1);
	
	//pthread_mutex_lock(&mut);
	if (pmax<PMAX){
		PMAX=pmax;
		right_cid=R_cid;
	}
	//pthread_mutex_unlock(&mut);
}



//读入预测文件
void main(){
	double start,finish;
	start=clock();

	FILE *fp;
	char arr[1024];
	char file[20];
	printf("输入测试文件\n");
	scanf("%s",file);
	//读取预测word
	if((fp=fopen(file,"r"))==NULL){
		printf("error");
		exit(0);
	}


	hash_table1_init();
	keep_data2("script2");
			
	get_data1("script1");

	char *a[100];
	char *title;
	float count=0;
	float Rcount=0;
	while(!feof(fp)){
		fgets(arr,1024,fp);
		if(!feof(fp)){
			char *ch=strtok(arr,"\n");
			char *buf=strtok(arr,"\t");
			int matchcid,rcid;
			matchcid=atoi(buf);
			title=strtok(NULL,"\t");
			printf("%s	真正的类目：%d \n",title,matchcid);
			count++;
	        int i=0;
	        char *buffer=strtok(title," ");
	        while(buffer!=NULL){
				a[i++]=buffer;
		        buffer=strtok(NULL," ");
			}
			int wordcount=i;
			//
		
			pthread_mutex_init(&mut,NULL);
			pthread_mutex_init(&mut1,NULL);
			
			int part=c_count/10+1;
			pthread_t t[10];
			ARG b[10];
			for(i=0;i<10;i++){
				pmax=0;
				b[i].sta=i*part;
				if(((i+1)*part-1)>=c_count)
					b[i].end=c_count-1;
				else 
					b[i].end=(i+1)*part-1;
				b[i].wordcount=wordcount;
				int j;
				for(j=0;j<wordcount;j++){
					b[i].word[j]=a[j];
				}
				pthread_create(&t[i],NULL,pthread,(void*)&b[i]);
			}
			for(i=0;i<10;i++){
				pthread_join(t[i],NULL);
			}
			/////////////////////////////////////////////////////////////

			printf("预测的类目：%d\n",right_cid);
			PMAX=0;
			if(right_cid==matchcid)
				Rcount++;
			//else
			//	printf("预测的类目：%d	应属于的类目:%d\n",right_cid,matchcid);
		}
	}
	float R=(Rcount/count)*100;
	printf("正确率：%f%\n",R);
	finish=clock();
	printf("%f seconds\n",(finish-start)/CLOCKS_PER_SEC);
}
	


/*
void main(){
	char str[100];
	FILE *fp;
	char* a[100];
	int i,j,m,count=1;
	fp=fopen("test","r");
	while(!feof(fp)){
		fgets(str,sizeof(str),fp);
		int length=strlen(str);
	//统计word数目count
	for(i=0;i<length;i++){
		if(str[i]=='\t'){
			count++;
			i++;
		}
	}
	if(!feof(fp)){
	      	a[0]=strtok(str,"\t");
			for(m=1;m<count;m++)
			{a[m]=strtok(NULL,"\t");}
		}
	}

	//将词典p(cid)存入哈希表
	hash_table_init();
	keep_data1("script1");
	
	//将词典p(term/cid)存入哈希表
	hash_table1_init();
	keep_data2("script2");
    int k;
	float pmax=-100;
	int rcid;
	for(k=0;k<MAX_SIZE;++k){
		if(hashTable[k]){
			Node* p1=hashTable[k];
			while(p1){
				float sum=p1->Value;
				for(j=0;j<count;j++){
					//如果单词wordj在词典2中存在，找到wordj地址
					if(hash_table1_lookup(a[j])){
						Listnode *p2=hash_table1_lookup(a[j]);
			//找到两个词典中相同的cid项求出p(cid)*p(wordj/cid)(j=0:count-1)
						while(p2){
							if((hash_table1_lookup(a[j]))->Cid==atoi(p1->Key)){
								sum=sum+((hash_table1_lookup(a[j]))->Value);
								break;
							}
							else
								p2=p2->next;
						}
				}
				}
				//将最大的概率和存入pmax，并记录正确的cid
						if(sum>pmax){
					pmax=sum;
					rcid=atoi(p1->Key);
					}
				p1=p1->next;
			}
		}
	}
	printf("正确的类目cid:%d  概率：log(p(cid/title))=%f\n",rcid,pmax);
	hash_table_release();
	hash_table1_release();
}
*/
