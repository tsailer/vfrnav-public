//
// C++ Interface: alignedalloc
//
// Description:
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef ALIGNEDALLOC_H
#define ALIGNEDALLOC_H

template<class T>
class aligned_allocator
{
public:
	typedef size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef T *pointer;
	typedef const T *const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

	template<class U>
	struct rebind
	{
		typedef aligned_allocator<U> other;
	};

	pointer address(reference value) const {
		return &value;
	}

	const_pointer address(const_reference value) const {
		return &value;
	}

	aligned_allocator() {
	}

	aligned_allocator(const aligned_allocator& ) {
	}

	template<class U>
	aligned_allocator(const aligned_allocator<U>& )	{
	}

	~aligned_allocator() {
	}

	size_type max_size() const {
		return (std::numeric_limits<size_type>::max)();
	}

	pointer allocate(size_type num, const void *hint = 0) {
#ifdef __WIN32__
		return static_cast<pointer>(_aligned_malloc(num * sizeof(T), 32));
#else
		void *memptr = (void *)hint;
		if (posix_memalign(&memptr, 32, num * sizeof(T)))
			return 0;
		return static_cast<pointer>(memptr);
#endif
	}

	void construct(pointer p, const T& value) {
		::new(p) T(value);
	}

	void destroy(pointer p)	{
		p->~T();
	}

	void deallocate(pointer p, size_type /*num*/) {
		free(p);
	}

	bool operator!=(const aligned_allocator<T>& ) const { return false; }

	bool operator==(const aligned_allocator<T>& ) const { return true; }
};

#endif /* ALIGNEDALLOC_H */
