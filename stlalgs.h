/*
ranges are similar to slices
come out of boost, although in there ranges would cover both slices and sequences, and vectors, ...

"The motivation for the Range concept is that there are many useful
Container-like types that do not meet the full requirements of
Container, and many algorithms that can be written with this reduced
set of requirements. In particular, a Range does not necessarily

    - own the elements that can be accessed through it,
    - have copy semantics,

": From boost  (forward range, bidirectional range, random access range)

following from:
https://www.fluentcpp.com/2017/01/12/ranges-stl-to-the-next-level/
ideas seem to be mostly from boost

transform_iterator -> delayed_seq
filter_iterator -> skips over elements not satisfying a predicate, only makes sense for linear iterator, not random access iterator.

view adaptors

view::transform adaptor

std::vector numbers = { 1, 2, 3, 4, 5 };
auto range = numbers | view::transform(multiplyBy2);
ranges::accumulate(numbers | view::transform(multiplyBy2), 0);
ranges::accumulate(numbers | view::filter(isEven), 0);
ranges::accumulate(numbers | view::filter(isEven) | view::transform(multiplyBy2), 0);

This has many similarities to lazy sequences in haskell.  There is a pipelining from one to the next.   I guess it could not only filter, but actually add new elelments (e.g. duplicate).

Note that not exactly like delayed seq since they transform an
existing iterator rather than starting from indices.  Although I guess
can start with iota.
*/

template<class Seq, class IntegerPred>
bool count_if_index(size_t n, IntegerPred p) {
  auto BS = pbbs::delayed_seq<bool>(n, [&] (size_t i) -> size_t {
      return p(i);});
  return pbbs::reduce(BS, pbbs::addm<size_t>());
}

template<class Seq, class UnaryPred>
size_t find_if_index(size_t n, IntegerPred p, granularity=1000) {
  size_t i;
  for (i = 0; i < std::min(granularity, n); i++)
    if (p(i)) return i;
  if (i < n) return i;
  size_t start = granularity;
  while (start < n) {
    size_t end = std::min(n,start+granularity);
    auto f = [&] (size_t i) -> size_t {
      return p(i+start) ? n : i+start;};
    i = pbbs::reduce(delayed_seq<size_t>(end-start, f),
		     minm<size_t>());
    if (i < n) return i;
    start += granularity;
    granularity *= 2;
  }
  return n;
}

template<class Seq, class UnaryPred>
bool count_if(Seq S, UnaryPred p) {
  return count_if_index(S.size(), [&] (size_t i) {return p(S[i]);});}

template<class Seq, class T>
bool count(Seq S, T value) {
  return count_if_index(S.size(), [&] (size_t i) {return S[i] == value;});}

template<class Seq, class UnaryPred>
bool all_of(Seq S, UnaryPred p) { return count_if(S, p) == S.size();}

template<class Seq, class UnaryPred>
bool any_of(Seq S, UnaryPred p) { return count_if(S, p) > 1;}

template<class Seq, class UnaryPred>
bool none_of(Seq S, UnaryPred p) { return count_if(S, p) == 0;}

template<class Seq, class UnaryPred>
size_t find_if(Seq S, UnaryPred p) {
  return find_if_index(S.size, [&] (size_t i) {return p(S[i]);});}

template<class Seq, class T>
size_t find(Seq S, T value) {
  return find_if(S, [&] (T x) {return x == value;});}

template<class Seq, class BinaryPred>
size_t adjacent_find(Seq S, BinaryPred pred) {
  return find_if_index(S.size()-1, [&] (size_t i) {
      return S[i] == S[i+1];});}

template<class Seq, class BinaryPred>
size_t mismatch(Seq S1, Seq S2, BinaryPred pred) {
  return find_if_index(S.size(), [&] (size_t i) {
      return S1[i] !== S2[i];});}

template<class Seq, class BinaryPred>
size_t search(Seq S1, Seq S2, BinaryPred pred) {
  return find_if_index(S.size(), [&] (size_t i) {
      size_t j;
      for (j=0; j < S2.size(); j++)
	if (S1[i+j] != S2[j]) break;
      return (j == S2.size());
    });}
 
template <class Seq1, class Seq2, class BinaryPred>
bool equal(Seq1 s1, Seq2 s2, BinaryPred p) {
  return count_if_index(s1.size(), [&] (size_t i) {
      return p(s1[i],s2[i]);});}

template <class Seq1, class Seq2, class BinaryPred>
bool equal(Seq1 s1, Seq2 s2) {
  return count_if_index(s1.size(), [&] (size_t i) {
      return s1[i] == s2[i];});}
  
template <class Seq1, class Seq2, class Compare>
bool lexicographical_compare(Seq1 s1, Seq2 s2, Compare less) {
  auto s = delayed_seq(s1.size(), [&] (size_t i) {
      return less(s1[i], s2[i]) ? -1 : (less(s2[i], s1[i]) : 1 : 0);});
  auto f = [&] (char a, char b) { return (a == 0) ? b : a;};
  return reduce(s, make_monoid(f, (char) 0)) == -1; 
}

template <class Seq, class Eql>
sequence<typename Seq::value_type>
unique (Seq s, Eql eq) {
  sequence<bool> b(s.size(), [&] (size_t i) {
      return (i == 0) || !(s[i] == s[i-1]);});
  return pack(s, b);
}

// needs to return location, and take comparison
template <class Seq, class Compare>
size_t min_element(Seq S, Compare comp) {
  auto SS = delayed_seq<size_t>(S.size(), [&] (size_t i) {
      return i;});
  auto f = [&] (size_t l, size_t r) {
    return (!comp(S[r], S[l]) ? l : r);};
  return pbbs::reduce(S, make_monoid(f, (size_t) S.size()));
}

template <class Seq, class Compare>
size_t max_element(Seq S, Compare comp) {
  using T = Seq::value_type;
  return min_element(S, [&] (T a, T b) {return f(b, a);});
}

template <class Seq>
std::pair<size_t, size_t>
minmax_element(Seq S) {
  size_t n = S.size();
  using P = std::pair<size_t, size_t>
  auto SS = delayed_seq(S.size(), [&] (size_t i) {
      make_pair(i,i);});
  auto f = [&] (P l, P r) {
    return (P(!comp(S[r.first], S[l.first]) ? l.first : r.first,
	      !comp(S[l.second], S[r.second]) ? l.second : r.second));};
  return pbbs::reduce(SS, make_monoid(f, P(n,n)));
}

template <class Seq>
sequence<typename Seq::value_type> reverse(Seq S) {
  size_t n = S.size();
  return sequence<typename Seq::value_type>(S.size(), [&] (size_t i) {
      return S[n-i-1];});}


/*

Most of these are from the boost libraries, but the boost versions take ranges (as in slices).

for_each
copy
copy_if
copy_n
move
fill
fill_n (takes iter + len)
transform (parallel_for)
generate (applies same function to each location)
generate_n
remove (for a value)
remove_if (for a function)
remove_copy (as remove but to a new location)
remove_copy_if
replace
replace_if
swap_ranges
reverse
reverse_copy
rotate
rotate_copy
shift_left
shift_right
is_partitioned
partition
partition_copy
stable_partition
is_sorted
is_sorted_until
sort
partial_sort
partial_sort_copy
stable_sort
nth_element (finds n_th element and paritions on it)
merge
inplace_merge
includes
set_difference
set_intersection
set_symmetric_difference
set_union
is_heap
is_heap_until
adjacent_difference
reduce (requires commutativity)
exclusive_scan
inclusive_scan
transform_reduce (map reduce)
transform_exclusive_scan
transform_inclusive_scan
uninitialized_copy
uninitialized_copy_n
uninitialized_fill
uninitialized_fill_n
uninitialized_move
uninitialized_move_n
uninitialized_default_construct
uninitialized_default_construct_n
uninitialized_value_construct
uninitialized_value_construct_n
destroy
destroy_n

Following do not have parallel versions
random_shuffle
sample
make_heap
sort_heap
is_permutaton
iota
accumulate (does not require associativity
inner_product

*/	  

/*
Duck Typing

constraints and concepts
-fconcepts in gcc >= 6.1

examples from: www.stroustrup.com/good_concepts.pdf

template<typename T>
concept bool Sequence =
  requires(T t) {
    typename Value_type<T>;
    typename Iterator_of<T>;
    { begin(t) } -> Iterator_of<T>;
    { end(t) } -> Iterator_of<T>;
    requires Input_iterator<Iterator_of<T>>;
    requires Same_type<Value_type<T>,Value_type<Iterator_of<T>>>;
};

template<typename T>
concept bool Equality_comparable =
  requires (T a, T b) {
    { a == b } -> bool;
    { a != b } -> bool;
};

template<typename T>
concept bool Sortable =
  Sequence<T> &&
  Random_access_iterator<Iterator_of<T>> &&
  Less_than_comparable<Value_type<T>>;

template<typename T, typename U>
concept bool Equality_comparable =
  requires (T a, U b) {
    { a == b } -> bool;
    { a != b } -> bool;
    { b == a } -> bool;
    { b != a } -> bool;
};


template<typename T>
concept bool Number = requires(T a, T b) {
  { a+b } -> T;
  { a-b } -> T;
  { a*b } -> T;
  { a/b } -> T;
  { -a } -> T;
  { a+=b } -> T&;
  { a-=b } -> T&;
  { a*=b } -> T&;
  { a/=b } -> T&;
  { T{0} };
  ...
};

template <Sequence S, typename T>
    requires Equality_comparable<Value_type<S>, T>
Iterator_of<S> find(S& seq, const T& value);

Or, equivalently:

template <typename S, typename T>
    requires Sequence<S> && Equality_comparable<Value_type<S>, T>
Iterator_of<S> find(S& seq, const T& value);

If only one constraint:

void sort(Sortable& s);


Problem of accidental matching.  The longer the list the better.
Avoid single property concepts.

class My_number { /* ... */ };
static_assert(Number<My_number>);

*/
