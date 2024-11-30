#ifndef _UTILS_H
#define _UTILS_H

#define LOOP_L_N(itr, N) for (size_t itr=0; itr < N; ++itr)
#define LOOP_LE_N(itr, N) for (size_t itr=0; itr <= N; ++itr)
#define NOMORE(v, n) if ((n) < (v)) (v) = (n);
#define NOLESS(v, n) if ((n) > (v)) (v) = (n);
#define _MIN(l, r) (l < r)?(l):(r)
#define _MAX(l, r) (l > r)?(l):(r)

#endif