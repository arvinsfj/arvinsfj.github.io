#常用数组排序算法

> *作者：Arvin 日期：2016年8月26日*

---------------------------------

>BEGIN

### 一、冒泡排序
---------------------------------
    void bubble_sort(int* L, int size)
    {
        int tmp, i, j;
        for (i = 0; i < size - 1; i++) {
            for (j = 0; j < size - 1 - i; j++) {
               if (L[j] > L[j+1]) {
                    tmp = L[j];
                    L[j] = L[j+1];
                    L[j+1] = tmp;
                }
            }
        }
    }
    
### 二、插入排序
---------------------------------
    void insert_sort(int* L, int size)
    {
	    int tmp, i, j;
	    for (i = 1; i < size; ++i) {
		    tmp = L[i];
		    for (j = i-1; j >= 0 && L[j] > tmp; j--) {
			    L[j+1] = L[j];
		    }
		    L[j+1] = tmp;
	    }
    }

### 三、快速排序
---------------------------------
    void quick_sort(int* L, int left, int right)
    {
        if(left >= right){
            return ;
        }
        int i = left, j = right, key = L[left];
        while(i < j){
            while(i < j && key <= L[j]){
                j--;
            }
            L[i] = L[j];
            while(i < j && key >= L[i]){
                i++;
            }
            L[j] = L[i];
        }
        L[i] = key;
        quick_sort(L, left, i - 1);
        quick_sort(L, i + 1, right);
    }

### 四、选择排序
---------------------------------
    void select_sort(int* L, int size)
    {
        int tmp,i,j,min;
        for(i = 0; i < size - 1; i++){
            min = i;
            for(j = i + 1; j < size; j++){
                if(L[min] > L[j]){
                    min = j;
                }
            }
            if(min != i){
                tmp = L[min];
                L[min] = L[i];
                L[i] = tmp;
            }
        }
    }

### 五、其他代码
---------------------------------
    void print_array(int* L, int size) {
	    printf("array: ");
	    for (int i = 0; i < size; ++i) {
		    printf("%d ", L[i]);
	    }
	    printf("\n");
    }

    int main (int argc, char** argv) {
	    int L[] = {18, 7, 5, 8, 99};
	    int size = sizeof(L) / sizeof(int);
	    //insert_sort(L, size);
      //bubble_sort(L, size);
      //select_sort(L, size);
      quick_sort(L, 0, 4);
      print_array(L, size);
	    return 0;
    }

>END

[代码下载](documents/array_sort.zip)

