
        Parallel generation of fractals using Julia and Mandelbrot sets

    The main idea of the project was to paralellize the generation of Julia and
Mandelbrot fractals starting from the sequential implementation of those algos.
The bread and butter of the project was creating a 'thread function' which every
given thread of CPU should execute in order to calculate the result and to
write it.

    As a starting point I had to come up with a way in which the algorithm
can be paralellized. The algos for both fractals can be divided in a number of
necessary steps. The total number of necessary steps is given by the width and
height of the complex subspace in which I want to create the fractals and a
resolution(the iteration step). This total number can be divided in chunks which
can be calculated in parallel. For both algos I divided the complex subspace
in chunks along the OY axis.
    For every thread I calculate the start and the end point along the height(OY),
so if I have N iterations along OY and P threads, every thread will calculate
N / P iterations. Start and end points for every thread are calculated after
by formula:
          start = ID * height / P;
          end = min((ID + 1) * height / P, height);
          where ID = the id of the current thread
                height = total number of necessary iterations along OX
                P = total number of threads
    The same idea was applied for writing the result into a PGM file.

    Given the fact that the construction of my resulted fractals and then the
convertion to PGM image are done in the same thread function, the result
matrix is a critical region to which a thread should not have access unless all
the other threads made same modifications. So the implementation should be
synchronized. I put barriers between calculating result of Julia algo and
writing it to PGM and same for Mandelbrot. Julia and Mandelbrot algos are
implemented in the same thread function but there is no need for barrier between
those 2 algos because they work with different variables as those are declared
globally.

    I declared the necessary variables globally for easing the way they can be
accessed in my thread function.
