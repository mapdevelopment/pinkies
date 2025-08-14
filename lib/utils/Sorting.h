#include <AceSorting.h>
#include <math.h>

using ace_sorting::quickSortMiddle;

float get_dominant_cluster_average(
    const int N, 
    float array[],
    const float tolerance
) {
    quickSortMiddle(array, N);

    float best_sum = 0.0f;
    int best_count = 0;

    float curr_sum = array[0];
    int curr_count = 1;

    for (int i = 1; i < N; i++) {
        if (fabsf(array[i] - array[i - 1]) < tolerance) {
            curr_sum += array[i];
            curr_count++;
        } else {
            if (curr_count > best_count) {
                best_sum = curr_sum;
                best_count = curr_count;
            }

            curr_sum = array[i];
            curr_count = 1;
        }
    }

    if (curr_count > best_count) {
        best_sum = curr_sum;
        best_count = curr_count;
    }

    return best_sum / best_count;
}