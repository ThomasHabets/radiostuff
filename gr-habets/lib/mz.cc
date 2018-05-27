// Taken from https://physics.princeton.edu/pulsar/k1jt/JT65code.tgz
#include <math.h>
#include <stdio.h>
#include <float.h>
#include <limits.h>
#include <stdlib.h>
#define MAX_RANDOM LONG_MAX    // Maximum value of random() 
// #define PRINT_SYNDROME 1

int i;
int m, n, length, k, t, t2, d, red;
int init_zero;
int p[10];
int alpha_to[64], index_of[64], g[63];
int recd[63], data[12], b[51];
int numera;
int era[50];

void read_p(void);
void generate_gf(void);
void gen_poly(void);
void encode_rs(void);
void bpsk_awgn(void);
void decode_rs(void);
int weight(int word);

void rs_init_(void)
{
  read_p();        // Read m */
  generate_gf();   // Construct the Galois Field GF(2^m) */
  gen_poly();      // Compute the generator polynomial of RS code */
}

void rs_encode_(int *dgen, int *sent)
{
  for(i=0; i<k; i++) {
    data[i]=dgen[i];
  }
  encode_rs();    // encode data ==> compute parity symbols b[i]

  // Move partity symbols into recd[] array
  for (i = 0; i < length - k; i++) sent[i] = b[i];  
  for (i = 0; i < k; i++) sent[i + length - k] = data[i];
}

void rs_decode_(int *recd0, int *era0, int *numera0, int *decoded)
{
  numera=*numera0;
  //  recd0[21]=15;
  for(i=0; i<length; i++) recd[i]=recd0[i];
  if(numera) for(i=0; i<numera; i++) era[i]=era0[i];
  decode_rs();
  for(i=0; i<length; i++) decoded[i]=recd[i+length-k];
}    

void read_p()
//      Read m, the degree of a primitive polynomial p(x) used to compute the
//      Galois field GF(2**m). Get precomputed coefficients p[] of p(x). Read
//      the code length.
{
  int i, ninf;

// Define constant parameters:
  m=6; length=63; red=51; init_zero=3;
  k = length - red;
  t = red/2;
  t2 = 2*t;

  for (i=1; i<m; i++)
    p[i] = 0;
  p[0] = p[m] = 1;
  if (m == 2)             p[1] = 1;
  else if (m == 3)        p[1] = 1;
  else if (m == 4)        p[3] = 1;
  // else if (m == 4)        p[1] = 1;  // Commented out to match example p. 68
  else if (m == 5)        p[2] = 1;
  else if (m == 6)        p[1] = 1;
  else if (m == 7)        p[1] = 1;
  else if (m == 8)        p[4] = p[5] = p[6] = 1;
  else if (m == 9)        p[4] = 1;
  else if (m == 10)       p[3] = 1;
  else if (m == 11)       p[2] = 1;
  else if (m == 12)       p[3] = p[4] = p[7] = 1;
  else if (m == 13)       p[1] = p[3] = p[4] = 1;
  else if (m == 14)       p[1] = p[11] = p[12] = 1;
  else if (m == 15)       p[1] = 1;
  else if (m == 16)       p[2] = p[3] = p[5] = 1;
  else if (m == 17)       p[3] = 1;
  else if (m == 18)       p[7] = 1;
  else if (m == 19)       p[1] = p[5] = p[6] = 1;
  else if (m == 20)       p[3] = 1;
  //  printf("Primitive polynomial of GF(2^%d), (LSB first)   p(x) = ",m);
  n = 1;
  for (i = 0; i <= m; i++) 
    {
      n *= 2;
      //      printf("%1d", p[i]);
    }
  //  printf("\n");
  n = n / 2 - 1;
}



void generate_gf()
// generate GF(2^m) from the irreducible polynomial p(X) in p[0]..p[m]
//
// lookup tables:  log->vector form           alpha_to[] contains j=alpha**i;
//                 vector form -> log form    index_of[j=alpha**i] = i
// alpha=2 is the primitive element of GF(2^m)
{
register int i, mask; 
  mask = 1; 
  alpha_to[m] = 0; 
  for (i=0; i<m; i++)
   { 
     alpha_to[i] = mask; 
     index_of[alpha_to[i]] = i; 
     if (p[i]!=0)
       alpha_to[m] ^= mask; 
     mask <<= 1; 
   }
  index_of[alpha_to[m]] = m; 
  mask >>= 1; 
  for (i=m+1; i<n; i++)
   { 
     if (alpha_to[i-1] >= mask)
        alpha_to[i] = alpha_to[m] ^ ((alpha_to[i-1]^mask)<<1); 
     else alpha_to[i] = alpha_to[i-1]<<1; 
     index_of[alpha_to[i]] = i; 
   }
  index_of[0] = -1; 

#ifdef PRINT_GF
printf("Table of GF(%d):\n",n);
printf("----------------------\n");
printf("   i\tvector \tlog\n");
printf("----------------------\n");
for (i=0; i<=n; i++)
printf("%4d\t%4x\t%4d\n", i, alpha_to[i], index_of[i]);
#endif
 }


void gen_poly()
// Compute the generator polynomial of the t-error correcting, length
// n=(2^m -1) Reed-Solomon code from the product of (X+alpha^i), for
// i = init_zero, init_zero + 1, ..., init_zero+length-k-1
{
register int i,j; 

   g[0] = alpha_to[init_zero];  //  <--- vector form of alpha^init_zero
   g[1] = 1;     // g(x) = (X+alpha^init_zero)
   for (i=2; i<=length-k; i++)
    { 
      g[i] = 1;
      for (j=i-1; j>0; j--)
        if (g[j] != 0)  
          g[j] = g[j-1]^ alpha_to[(index_of[g[j]]+i+init_zero-1)%n]; 
        else 
          g[j] = g[j-1]; 
      g[0] = alpha_to[(index_of[g[0]]+i+init_zero-1)%n];
    }
   // convert g[] to log form for quicker encoding 
   for (i=0; i<=length-k; i++)  g[i] = index_of[g[i]]; 
#ifdef PRINT_POLY
printf("Generator polynomial (independent term first):\ng(x) = ");
for (i=0; i<=length-k; i++) printf("%5d", g[i]);
printf("\n");
#endif
}


void encode_rs()
// Compute the 2t parity symbols in b[0]..b[2*t-1]
// data[] is input and b[] is output in polynomial form.
// Encoding is done by using a feedback shift register with connections
// specified by the elements of g[].
{
   register int i,j; 
   int feedback; 

   for (i=0; i<length-k; i++)   
     b[i] = 0; 
   for (i=k-1; i>=0; i--)
    {
    feedback = index_of[data[i]^b[length-k-1]]; 
      if (feedback != -1)
        { 
        for (j=length-k-1; j>0; j--)
          if (g[j] != -1)
            b[j] = b[j-1]^alpha_to[(g[j]+feedback)%n]; 
          else
            b[j] = b[j-1]; 
          b[0] = alpha_to[(g[0]+feedback)%n]; 
        }
       else
        { 
        for (j=length-k-1; j>0; j--)
          b[j] = b[j-1]; 
        b[0] = 0; 
        }
    }
}



void decode_rs()
{
   register int i,j,u,q;
   int elp[1026][1024], d[1026], l[1026], u_lu[1026], s[1025], forney[1025];
   int count=0, syn_error=0, tau[512], root[512], loc[512], z[513]; 
   int err[1024], reg[513], aux[513], omega[1025], phi[1025], phiprime[1025];
   int degphi, ell, temp;

   // Compute the syndromes

#ifdef PRINT_SYNDROME
   printf("\ns =         0 ");
#endif

   for (i=1; i<=t2; i++)
    { 
      s[i] = 0; 
      for (j=0; j<length; j++)
        if (recd[j]!=0)
          s[i] ^= alpha_to[(index_of[recd[j]]+(i+init_zero-1)*j)%n];
      // convert syndrome from vector form to log form  */
      if (s[i]!=0)  
        syn_error=1;         // set flag if non-zero syndrome => error
      //
      // Note:    If the code is used only for ERROR DETECTION, then
      //          exit program here indicating the presence of errors.
      //
      s[i] = index_of[s[i]]; 

#ifdef PRINT_SYNDROME
   printf("%4d ", s[i]);
#endif

    }

   if (syn_error)       // if syndromes are nonzero then try to correct
    {

     s[0] = 0;  // S(x) = 1 + s_1x + ...

      // TO HANDLE ERASURES

      if (numera)
      // if erasures are present, compute the erasure locator polynomial, tau(x)
        {
          for (i=0; i<=t2; i++) 
            { tau[i] = 0; aux[i] = 0;}

          aux[1] = alpha_to[era[0]];
          aux[0] = 1;       // (X + era[0]) 

          if (numera>1)

            for (i=1; i<numera; i++)
              {
              p[1] = era[i];
              p[0] = 0;
              for (j=0; j<2; j++)
                for (ell=0; ell<=i; ell++)
                  // Line below added 8/17/2003
                  if ((p[j] !=-1) && (aux[ell]!=0))
                    tau[j+ell] ^= alpha_to[(p[j]+index_of[aux[ell]])%n];
              if (i != (numera-1))
              for (ell=0; ell<=(i+1); ell++)
                {
                aux[ell] = tau[ell];
                tau[ell] = 0;
                }
              }

          else {
            tau[0] = aux[0]; tau[1] = aux[1];
          }

        // Put in index (log) form
        for (i=0; i<=numera; i++)
          tau[i] = index_of[tau[i]]; /* tau in log form */

#ifdef PRINT_SYNDROME
printf("\ntau =    ");
for (i=0; i<=numera; i++)
  printf("%4d ", tau[i]);
printf("\nforney = ");
#endif

        // Compute FORNEY modified syndrome:
        //            forney(x) = [ s(x) tau(x) + 1 ] mod x^{t2}

        for (i=0; i<=n-k; i++) forney[i] = 0;
    
        for (i=0; i<=n-k; i++)
          for (j=0; j<=numera; j++)
            if (i+j <= (n-k)) // mod x^{n-k+1}
              if ((s[i]!=-1)&&(tau[j]!=-1))
                forney[i+j] ^= alpha_to[(s[i]+tau[j])%n];

        forney[0] ^= 1; 

        for (i=0; i<=n-k; i++)
          forney[i] = index_of[forney[i]];

#ifdef PRINT_SYNDROME
for (i=0; i<=n-k; i++)
  printf("%4d ", forney[i]);
#endif

   }
   else // No erasures
    {
    tau[0]=0;
    for (i=1; i<=n-k; i++) forney[i] = s[i];
    }

#ifdef PRINT_SYNDROME
printf("\n");
#endif


  // --------------------------------------------------------------
  //    THE BERLEKAMP-MASSEY ALGORITHM FOR ERRORS AND ERASURES
  // --------------------------------------------------------------

      // initialize table entries
      d[0] = 0;                // log form
      d[1] = forney[numera+1]; // log form
      elp[0][0] = 0;           // log form 
      elp[1][0] = 1;           // vector form 
      for (i=1; i<t2; i++)
        { 
          elp[0][i] = -1;   // log form
          elp[1][i] = 0;    // vector form 
        }
      l[0] = 0; 
      l[1] = 0; 
      u_lu[0] = -1; 
      u_lu[1] = 0; 
      u = 0; 

      if (numera<t2) {  // If errors can be corrected

      do
      {
        u++; 
        if (d[u]==-1)
          { 
#ifdef PRINT_SYNDROME
printf("d[%d] is zero\n",u);
#endif
            l[u+1] = l[u]; 
            for (i=0; i<=l[u]; i++)
             {  
               elp[u+1][i] = elp[u][i]; 
               elp[u][i] = index_of[elp[u][i]]; 
             }
          }
        else
          // search for words with greatest u_lu[q] for which d[q]!=0
          { 
            q = u-1; 
            while ((d[q]==-1) && (q>0))    q--; 
            // have found first non-zero d[q] 
            if (q>0)
             { 
               j=q; 
               do
               { 
                 j--; 
                 if ((d[j]!=-1) && (u_lu[q]<u_lu[j]))
                   q = j; 
               } while (j>0); 
             }

#ifdef PRINT_SYNDROME
printf("u = %4d, q = %4d, d[q] = %4d d[u] = %4d\n", u, q, d[q],d[u]);
#endif

            // have now found q such that d[u]!=0 and u_lu[q] is maximum 
            // store degree of new elp polynomial 
            if (l[u]>l[q]+u-q)  l[u+1] = l[u]; 
            else  l[u+1] = l[q]+u-q; 

#ifdef PRINT_SYNDROME
printf("l[q] = %4d, l[u] = %4d\n", l[q], l[u]);
#endif

            // compute new elp(x) 
            // for (i=0; i<t2-numera; i++)    elp[u+1][i] = 0; 
            for (i=0; i<t2; i++)    elp[u+1][i] = 0; 
            for (i=0; i<=l[q]; i++)
              if (elp[q][i]!=-1)
                elp[u+1][i+u-q] = alpha_to[(d[u]+n-d[q]+elp[q][i])%n]; 
            for (i=0; i<=l[u]; i++)
              { 
                elp[u+1][i] ^= elp[u][i]; 
                elp[u][i] = index_of[elp[u][i]];   
              }

#ifdef PRINT_SYNDROME
printf("l[u+1] = %4d, elp[u+1] = ", l[u+1]);
for (i=0;  i<=l[u+1]; i++) printf("%4d ",index_of[elp[u+1][i]]);
printf("\n");
#endif

          }
        u_lu[u+1] = u-l[u+1]; 
        // compute (u+1)th discrepancy 
        // if (u<t2)    // no discrepancy computed on last iteration 
        if (u<(t2-numera)) // no discrepancy computed on last iteration 
        // --- if ( u < (l[u+1]+t-1-(numera/2)) ) 
          {
            // if (s[u+1]!=-1)
            if (forney[numera+u+1]!=-1)
              d[u+1] = alpha_to[forney[numera+u+1]]; 
            else
              d[u+1] = 0; 
#ifdef PRINT_SYNDROME
printf("discrepancy for u = %d: d[u+1] = %4d\n", u, index_of[d[u+1]]);
#endif

            for (i=1; i<=l[u+1]; i++)
              // if ((s[u+1-i]!=-1) && (elp[u+1][i]!=0))
              //   d[u+1] ^= alpha_to[(s[numera+u+1-i]
              if ((forney[numera+u+1-i]!=-1) && (elp[u+1][i]!=0)) {
                 d[u+1] ^= alpha_to[(forney[numera+u+1-i]
                                      + index_of[elp[u+1][i]])%n]; 
#ifdef PRINT_SYNDROME
printf("i=%d, forney[%d] = %4d, d[u+1] = %4d\n",i,numera+u+1-i,
                   forney[numera+u+1-i],index_of[d[u+1]]);
#endif
                }
            d[u+1] = index_of[d[u+1]];     // put d[u+1] into index form 

#ifdef PRINT_SYNDROME
printf("d[u+1] = %4d\n", d[u+1]);
#endif

          }
      // } while ((u<t2) && (l[u+1]<=t)); 
      } while ((u<(t2-numera)) && (l[u+1]<=((t2-numera)/2))); 
      }

      // else // case of 2t erasures
      // {
      //   elp[1][0] = 0;
      //   count = 0;
      // }

      u++; 

      if (l[u]<=t-numera/2)         // can correct errors
       {
         // put elp into index form 
         for (i=0; i<=l[u]; i++)   elp[u][i] = index_of[elp[u][i]]; 

	 /* printf("\nBM algorithm, after %d iterations:\nsigma = ", (u-1));
	    for (i=0; i<=l[u]; i++)   printf("%4d ", elp[u][i]);
	    printf("\n");
	 */

         // find roots of the error location polynomial 
         for (i=1; i<=l[u]; i++)
           reg[i] = elp[u][i]; 
         count = 0; 
         for (i=1; i<=n; i++)
          {  
             q = 1; 
             for (j=1; j<=l[u]; j++)
              if (reg[j]!=-1)
                { 
                  reg[j] = (reg[j]+j)%n; 
                  q ^= alpha_to[reg[j]]; 
                }
             if (!q)        // store root and error location number indices 
              { 
                root[count] = i;
                loc[count] = n-i; 
#ifdef PRINT_SYNDROME
printf("loc[%4d] = %4d\n", count, loc[count]);
#endif
                count++;
              }
           }

         if (count==l[u])    // no. roots = degree of elp hence <= t errors 
          {

            // Compute the errata evaluator polynomial, omega(x)

            forney[0] = 0;  // as a log, to construct 1+T(x)
            for (i=1; i<=t2; i++) 
              omega[i] = 0;
            for (i=0; i<=t2; i++)
              {
              for (j=0; j<=l[u]; j++)
                {
                if (i+j <= t2) // mod x^{t2}
                  if ((forney[i]!=-1) && (elp[u][j]!=-1))
                    omega[i+j] ^= alpha_to[(forney[i]+elp[u][j])%n];
                }
              }
            for (i=0; i<=t2; i++)
              omega[i] = index_of[omega[i]];

#ifdef PRINT_SYNDROME
printf("\nomega =    ");
for (i=0; i<=t2; i++)   printf("%4d ", omega[i]);
printf("\n");
#endif


           // Compute the errata locator polynomial, phi(x)

           degphi = numera+l[u];

           for (i=1; i<=degphi; i++) phi[i] = 0;
           for (i=0; i<=numera; i++)
             for (j=0; j<=l[u]; j++)
               if ((tau[i]!=-1)&&(elp[u][j]!=-1))
                 phi[i+j] ^= alpha_to[(tau[i]+elp[u][j])%n];
           for (i=0; i<=degphi; i++)
             phi[i] = index_of[phi[i]];

#ifdef PRINT_SYNDROME
printf("phi =      ");
for (i=0; i<=degphi; i++)   printf("%4d ", phi[i]);
printf("\n");
#endif


           // Compute the "derivative" of phi(x): phiprime

           for (i=0; i<=degphi; i++) phiprime[i] = -1; // as a log
           for (i=0; i<=degphi; i++)
             if (i%2)  // Odd powers of phi(x) give terms in phiprime(x)
               phiprime[i-1] = phi[i];

#ifdef PRINT_SYNDROME
printf("phiprime = ");
for (i=0; i<=degphi; i++)   printf("%4d ", phiprime[i]);
printf("\n\n");
#endif

// printf("**** count = %d\n", count);

            if (numera)
            // Add erasure positions to error locations
            for (i=0; i<numera; i++) {
              loc[count+i] = era[i];
              root[count+i] = (n-era[i])%n;
              }


           // evaluate errors at locations given by errata locations, loc[i]
           //                             for (i=0; i<l[u]; i++)    
           for (i=0; i<degphi; i++)    
            { 
              // compute numerator of error term  
              err[loc[i]] = 0;
              for (j=0; j<=t2; j++)
                if ((omega[j]!=-1)&&(root[i]!=-1))
                  err[loc[i]] ^= alpha_to[(omega[j]+j*root[i])%n];

              // -------  The term loc[i]^{2-init_zero}
              if ((err[loc[i]]!=0)&&(loc[i]!=-1))
                 err[loc[i]] = alpha_to[(index_of[err[loc[i]]]
                                              +loc[i]*(2-init_zero+n))%n];

              if (err[loc[i]]!=0)
               { 
                 err[loc[i]] = index_of[err[loc[i]]];
                 // compute denominator of error term 
                 q = 0;             
                 for (j=0; j<=degphi; j++)
                   if ((phiprime[j]!=-1)&&(root[i]!=-1))
                     q ^= alpha_to[(phiprime[j]+j*root[i])%n];

                 // Division by q
                 err[loc[i]] = alpha_to[(err[loc[i]]-index_of[q]+n)%n];

#ifdef PRINT_SYNDROME
printf("errata[%4d] = %4d (%4d) \n",loc[i],index_of[err[loc[i]]],err[loc[i]]);
#endif

                 recd[loc[i]] ^= err[loc[i]];  

               }
            }

	   /*
          printf("\nCor =");
          for (i=0; i<length; i++) {
             printf("%4d ", index_of[recd[i]]);
             }
          printf("\n     ");
          for (i=0; i<length; i++) {
             printf("%4d ", recd[i]);
             }
          printf("\n");
	   */

          }
         else    // no. roots != degree of elp => >t errors and cannot solve
          ;
       }
     else         // elp has degree has degree >t hence cannot solve 
       ;
    }
   else       // no non-zero syndromes => no errors: output received codeword
     //If we get here, there are certainly errors.  However, there may
     //very well be errors anyway, that are not detected here.
     ;
}



int weight(int word)
{
  int i, res;
  res = 0;
  for (i=0; i<m; i++)
    if ( (word>>i) & 0x01 ) res++;
  return(res);
}

