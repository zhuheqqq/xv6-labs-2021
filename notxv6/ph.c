#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#define NBUCKET 5
#define NKEYS 100000

pthread_mutex_t locks[NBUCKET];

struct entry {
  int key;
  int value;
  struct entry *next;
};
struct entry *table[NBUCKET];
int keys[NKEYS];
int nthread = 1;

//提供时间戳
double
now()
{
 struct timeval tv;//存储时间戳
 gettimeofday(&tv, 0);//获取当前时间戳
 return tv.tv_sec + tv.tv_usec / 1000000.0;//返回当前时间秒数
}

//在链表中插入新的节点  头插
static void 
insert(int key, int value, struct entry **p, struct entry *n)
{
  //使用malloc分配内存以存储新的entry节点
  struct entry *e = malloc(sizeof(struct entry));
  //设置新的节点的键值对和下一个节点的指针
  e->key = key;
  e->value = value;
  e->next = n;

  //更新传入的指针，使其指向新插入的节点
  *p = e;
}

//在哈希中插入或更新键值对
static 
void put(int key, int value)
{
  int i = key % NBUCKET;

  // is the key already present?判断键是否已经存在于哈希桶中
  pthread_mutex_lock(&locks[i]);
  struct entry *e = 0;
  for (e = table[i]; e != 0; e = e->next) {
    if (e->key == key)
      break;
  }
  if(e){
    // update the existing key.
    e->value = value;
  } else {
    // the new is new.
    insert(key, value, &table[i], table[i]);
  }
  pthread_mutex_unlock(&locks[i]);
}
//查找特定键对应的节点
static struct entry*
get(int key)
{
  int i = key % NBUCKET;

  //pthread_mutex_lock(&locks[i]);
  struct entry *e = 0;
  for (e = table[i]; e != 0; e = e->next) {
    if (e->key == key) break;
  }

  //pthread_mutex_unlock(&locks[i]);

  return e;
}

//线程函数 用于在哈希表中插入键值对
static void *
put_thread(void *xa)
{
  int n = (int) (long) xa; // thread number
  int b = NKEYS/nthread;

  for (int i = 0; i < b; i++) {
    put(keys[b*n + i], n);
  }

  return NULL;
}

//线程函数用于在哈希表中查找特定键对应的节点
static void *
get_thread(void *xa)
{
  int n = (int) (long) xa; // thread number
  int missing = 0;

  for (int i = 0; i < NKEYS; i++) {
    struct entry *e = get(keys[i]);
    if (e == 0) missing++;
  }
  printf("%d: %d keys missing\n", n, missing);
  return NULL;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  double t1, t0;


  if (argc < 2) {
    fprintf(stderr, "Usage: %s nthreads\n", argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);
  assert(NKEYS % nthread == 0);

  //加上锁的初始化就会段错误  but why？？
  //因为i小于的宏定义给错了
  for(int i=0;i<NBUCKET;i++)
  {
    pthread_mutex_init(&locks[i],NULL);
  }

  for (int i = 0; i < NKEYS; i++) {
    keys[i] = random();
  }

  //
  // first the puts
  //
  t0 = now();
  for(int i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, put_thread, (void *) (long) i) == 0);
  }
  for(int i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  t1 = now();

  printf("%d puts, %.3f seconds, %.0f puts/second\n",
         NKEYS, t1 - t0, NKEYS / (t1 - t0));

  //
  // now the gets
  //
  t0 = now();
  for(int i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, get_thread, (void *) (long) i) == 0);
  }
  for(int i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  t1 = now();

  printf("%d gets, %.3f seconds, %.0f gets/second\n",
         NKEYS*nthread, t1 - t0, (NKEYS*nthread) / (t1 - t0));
}
