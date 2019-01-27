/*
 * MVKVector.h
 *
 * Copyright (c) 2012-2019 Dr. Torsten Hans (hans@ipacs.de)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

//
// in case MVKVector should use std::vector
//
#if 0

template<typename T, size_t N = 0>
using MVKVectorInline = std::vector<T>;

template<typename T>
using MVKVectorDefault = std::vector<T>;

template<typename T>
using MVKVector = std::vector<T>;

#else

//
// MVKVector.h is a sequence container that (optionally) implements a small
// buffer optimization.
// It behaves similarly to std::vector, except until a certain number of
// elements are reserved, it does not use the heap.
// Like std::vector, MVKVector is guaranteed to use contiguous memory, so if the
// preallocated number of elements are exceeded, all elements are then in heap.
// MVKVector supports just the necessary members to be compatible with MoltenVK
// If C++17 will be the default in the future, code can be simplified quite
// a bit.
//
// Example:
//
//  MVKVectorInline<int, 3> vector;
//  vector.emplace_back( 1 );
//  vector.emplace_back( 2 );
//  vector.emplace_back( 3 );
//  // adding another element now reserves memory from heap
//  vector.emplace_back( 4 );
//
// If you don't need any inline storage use
//  MVKVectorDefault<int> vector; // this is essentially the same as using
//                                // std::vector
//
// Passing MVKVectorInline to a function would require to use the same template
// parameters that have been used for declaration. To avoid this MVKVectorInline
// is derived from MVKVector. If you want to pass MVKVectorInline to a function
// use MVKVector.
// Keep in mind MVKVector only supports iterating over the MVKVector, no other
// operation is supported.
//
#include "MVKVectorAllocator.h"
#include <type_traits>
#include <initializer_list>
#include <utility>


template<class Type> class MVKVector
{
  mvk_vector_allocator_base<Type> *alc_ptr;

public:
  class iterator
  {
    const MVKVector *vector;
    size_t           index;

  public:
    iterator() = delete;
    iterator( const size_t _index, const MVKVector &_vector ) : vector{ &_vector }, index{ _index } { }
    iterator &operator=( const iterator &it ) = delete;

    Type *operator->() const { return &vector->alc_ptr->ptr[index]; }
    Type &operator*()  const { return  vector->alc_ptr->ptr[index]; }
    operator Type*( )  const { return &vector->alc_ptr->ptr[index]; }

    bool operator==( const iterator &it ) const { return vector == it.vector && index == it.index; }
    bool operator!=( const iterator &it ) const { return vector != it.vector || index != it.index; }

    iterator& operator++()      {                 ++index; return *this; }
    iterator  operator++( int ) { auto t = *this; ++index; return t; }

    bool   is_valid()     const { return index < vector->size(); }
    size_t get_position() const { return index; }
  };

public:
  MVKVector() = delete;
  MVKVector( mvk_vector_allocator_base<Type> *a ) : alc_ptr{ a } { }
  virtual ~MVKVector() { }

  iterator begin()    const { return iterator( 0,               *this ); }
  iterator end()      const { return iterator( alc_ptr->size(), *this ); }
  size_t   size()     const { return alc_ptr->size(); }
  bool     empty()    const { return alc_ptr->size() == 0; }
  size_t   capacity() const { return alc_ptr->get_capacity(); }

  virtual const Type &operator[]( const size_t i ) const = 0;
  virtual void push_back( const Type &t ) = 0;
};


template<class Type> class MVKVector<Type *>
{
  mvk_vector_allocator_base<Type*> *alc_ptr;

public:
  class iterator
  {
    const MVKVector *vector;
    size_t           index;

  public:
    iterator() = delete;
    iterator( const size_t _index, const MVKVector &_vector ) : vector{ &_vector }, index{ _index } { }
    iterator &operator=( const iterator &it ) = delete;

    Type *operator->() const { return vector->alc_ptr->ptr[index]; }
    Type &operator*()  const { return vector->alc_ptr->ptr[index]; }
    operator Type*&()  const { return &vector->alc_ptr->ptr[index]; }

    bool operator==( const iterator &it ) const { return vector == it.vector && index == it.index; }
    bool operator!=( const iterator &it ) const { return vector != it.vector || index != it.index; }

    iterator& operator++()      {                 ++index; return *this; }
    iterator  operator++( int ) { auto t = *this; ++index; return t; }

    bool   is_valid()     const { return index < vector->size(); }
    size_t get_position() const { return index; }
  };

public:
  MVKVector() = delete;
  MVKVector( mvk_vector_allocator_base<Type*> *a ) : alc_ptr{ a } { }
  virtual ~MVKVector() { }

  iterator begin()    const { return iterator( 0,               *this ); }
  iterator end()      const { return iterator( alc_ptr->size(), *this ); }
  size_t   size()     const { return alc_ptr->size(); }
  bool     empty()    const { return alc_ptr->size() == 0; }
  size_t   capacity() const { return alc_ptr->get_capacity(); }

  virtual Type * const &operator[]( const size_t i ) const = 0;
  virtual void push_back( const Type *t ) = 0;
};


// this is the actual implementation of MVKVector
template<class Type, typename Allocator = mvk_vector_allocator_default<Type>> class MVKVectorImpl : public MVKVector<Type>
{
  friend class MVKVectorImpl;

  Allocator  alc;
  
public:
  class iterator
  {
    const MVKVectorImpl *vector;
    size_t               index;

  public:
    iterator() = delete;
    iterator( const size_t _index, const MVKVectorImpl &_vector ) : vector{ &_vector }, index{ _index } { }

    iterator &operator=( const iterator &it )
    {
      vector = it.vector;
      index  = it.index;
      return *this;
    }

    Type *operator->() { return &vector->alc.ptr[index]; }
    Type &operator*()  { return  vector->alc.ptr[index]; }
    operator Type*()   { return &vector->alc.ptr[index]; }

    bool operator==( const iterator &it ) const { return vector == it.vector && index == it.index; }
    bool operator!=( const iterator &it ) const { return vector != it.vector || index != it.index; }

    iterator& operator++()      {                 ++index; return *this; }
    iterator  operator++( int ) { auto t = *this; ++index; return t; }

    bool   is_valid()     const { return index < vector->alc.size(); }
    size_t get_position() const { return index; }
  };

private:
  // this is the growth strategy -> adjust to your needs
  size_t vector_GetNextCapacity() const
  {
    constexpr auto ELEMENTS_FOR_64_BYTES = 64 / sizeof( Type );
    constexpr auto MINIMUM_CAPACITY = ELEMENTS_FOR_64_BYTES > 4 ? ELEMENTS_FOR_64_BYTES : 4;
    const auto current_capacity = capacity();
    return MINIMUM_CAPACITY + ( 3 * current_capacity ) / 2;
  }

  void vector_Allocate( const size_t s )
  {
    const auto new_reserved_size = tm_max( s, size() );

    alc.allocate( new_reserved_size );
  }

  void vector_ReAllocate( const size_t s )
  {
    alc.re_allocate( s );
  }

public:
  MVKVectorImpl() : MVKVector<Type>{ &alc }
  {
  }

  MVKVectorImpl( const size_t n, const Type t ) : MVKVector<Type>{ &alc }
  {
    if( n > 0 )
    {
      alc.allocate( n );

      for( size_t i = 0; i < n; ++i )
      {
        alc.construct( &alc.ptr[i], t );
      }

      alc.num_elements_used = n;
    }
  }

  MVKVectorImpl( const MVKVectorImpl &a ) : MVKVector<Type>{ &alc }
  {
    const size_t n = a.size();

    if( n > 0 )
    {
      alc.allocate( n );

      for( size_t i = 0; i < n; ++i )
      {
        alc.construct( &alc.ptr[i], a.alc.ptr[i] );
      }

      alc.num_elements_used = n;
    }
  }

  template<typename U>
  MVKVectorImpl( const U &a ) : MVKVector<Type>{ &alc }
  {
    const size_t n = a.size();

    if( n > 0 )
    {
      alc.allocate( n );

      for( size_t i = 0; i < n; ++i )
      {
        alc.construct( &alc.ptr[i], a[i] );
      }

      alc.num_elements_used = n;
    }
  }

  MVKVectorImpl( MVKVectorImpl &&a ) : MVKVector<Type>{ &alc }, alc{ std::move( a.alc ) }
  {
  }

  MVKVectorImpl( std::initializer_list<Type> vector ) : MVKVector<Type>{ &alc }
  {
    if( vector.size() > capacity() )
    {
      vector_Allocate( vector.size() );
    }

    // std::initializer_list does not yet support std::move, we use it anyway but it has no effect
    for( auto &&element : vector )
    {
      alc.construct( &alc.ptr[alc.num_elements_used], std::move( element ) );
      ++alc.num_elements_used;
    }
  }

  ~MVKVectorImpl()
  {
  }

  template<typename U>
  MVKVectorImpl& operator=( const U &a )
  {
    static_assert( std::is_base_of<MVKVector<Type>, U>::value, "argument is not of type MVKVector" );

    if( this != reinterpret_cast<const MVKVector<Type>*>( &a ) )
    {
      const auto n = a.size();

      if( alc.num_elements_used == n )
      {
        for( size_t i = 0; i < n; ++i )
        {
          alc.ptr[i] = a.alc.ptr[i];
        }
      }
      else
      {
        if( n > capacity() )
        {
          vector_ReAllocate( n );
        }
        else
        {
          alc.template destruct_all<Type>();
        }

        for( size_t i = 0; i < n; ++i )
        {
          alc.construct( &alc.ptr[i], a[i] );
        }

        alc.num_elements_used = n;
      }
    }

    return *this;
  }

  MVKVectorImpl& operator=( MVKVectorImpl &&a )
  {
    alc.swap( a.alc );
    return *this;
  }

  bool operator==( const MVKVectorImpl &a ) const
  {
    if( alc.num_elements_used != a.alc.num_elements_used )
      return false;
    for( size_t i = 0; i < alc.num_elements_used; ++i )
    {
      if( alc[i] != a.alc[i] )
        return false;
    }
    return true;
  }

  bool operator!=( const MVKVectorImpl &a ) const
  {
    if( alc.num_elements_used != a.alc.num_elements_used )
      return true;
    for( size_t i = 0; i < alc.num_elements_used; ++i )
    {
      if( alc.ptr[i] != a.alc[i] )
        return true;
    }
    return false;
  }

  void swap( MVKVectorImpl &a )
  {
    alc.swap( a.alc );
  }

  void clear()
  {
    alc.template destruct_all<Type>();
  }

  void reset()
  {
    alc.deallocate();
  }

  iterator         begin()  const { return iterator( 0, *this ); }
  iterator         end()    const { return iterator( alc.num_elements_used, *this ); }
  size_t           size()   const { return alc.num_elements_used; }
  bool             empty()  const { return alc.num_elements_used == 0; }

  Type &at( const size_t i ) const
  {
    return alc.ptr[i];
  }

  const Type &operator[]( const size_t i ) const override
  {
    return alc.ptr[i];
  }

  Type &operator[]( const size_t i )
  {
    return alc.ptr[i];
  }

  const Type *data() const
  {
    return alc.ptr;
  }

  Type *data()
  {
    return alc.ptr;
  }

  size_t capacity() const
  {
    return alc.get_capacity();
  }

  const Type &front() const
  {
    return alc.ptr[0];
  }

  Type &front()
  {
    return alc.ptr[0];
  }

  const Type &back() const
  {
    return alc.ptr[alc.num_elements_used - 1];
  }

  Type &back()
  {
    return alc.ptr[alc.num_elements_used - 1];
  }

  void pop_back()
  {
    if( alc.num_elements_used > 0 )
    {
      --alc.num_elements_used;
      alc.destruct( &alc.ptr[alc.num_elements_used] );
    }
  }

  void reserve( const size_t new_size )
  {
    if( new_size > capacity() )
    {
      vector_ReAllocate( new_size );
    }
  }

  void assign( const size_t new_size, const Type &t )
  {
    if( new_size <= capacity() )
    {
      clear();
    }
    else
    {
      vector_Allocate( new_size );
    }

    for( size_t i = 0; i < new_size; ++i )
    {
      alc.construct( &alc.ptr[i], t );
    }

    alc.num_elements_used = new_size;
  }

  template <class InputIterator>
  void assign( InputIterator first, InputIterator last )
  {
    clear();

    while( first != last )
    {
      emplace_back( *first );
      ++first;
    }
  }

  void resize( const size_t new_size, const Type t = { } )
  {
    if( new_size == alc.num_elements_used )
    {
      return;
    }

    if( new_size == 0 )
    {
      clear();
      return;
    }

    if( new_size > alc.num_elements_used )
    {
      if( new_size > capacity() )
      {
        vector_ReAllocate( new_size );
      }

      while( alc.num_elements_used < new_size )
      {
        alc.construct( &alc.ptr[alc.num_elements_used], t );
        ++alc.num_elements_used;
      }
    }
    else
    {
      //if constexpr( !std::is_trivially_destructible<Type>::value )
      {
        while( alc.num_elements_used > new_size )
        {
          --alc.num_elements_used;
          alc.destruct( &alc.ptr[alc.num_elements_used] );
        }
      }
      //else
      //{
      //  alc.num_elements_used = new_size;
      //}
    }
  }

  // trims the capacity of the slist to the number of alc.ptr
  void shrink_to_fit()
  {
    alc.shrink_to_fit();
  }

  void erase( const iterator it )
  {
    if( it.is_valid() )
    {
      --alc.num_elements_used;

      for( size_t i = it.get_position(); i < alc.num_elements_used; ++i )
      {
        alc.ptr[i] = std::move( alc.ptr[i + 1] );
      }

      // this is required for types with a destructor
      alc.destruct( &alc.ptr[alc.num_elements_used] );
    }
  }

  // adds t before it and automatically resizes vector if necessary
  void insert( const iterator it, Type t )
  {
    if( !it.is_valid() || alc.num_elements_used == 0 )
    {
      push_back( std::move( t ) );
    }
    else
    {
      if( alc.num_elements_used == capacity() )
        vector_ReAllocate( vector_GetNextCapacity() );

      // move construct last element
      alc.construct( &alc.ptr[alc.num_elements_used], std::move( alc.ptr[alc.num_elements_used - 1] ) );

      // move the remaining elements
      const size_t it_position = it.get_position();
      for( size_t i = alc.num_elements_used - 1; i > it_position; --i )
      {
        alc.ptr[i] = std::move( alc.ptr[i - 1] );
      }

      alc.ptr[it_position] = std::move( t );
      ++alc.num_elements_used;
    }
  }

  void push_back( const Type &t ) override
  {
    if( alc.num_elements_used == capacity() )
      vector_ReAllocate( vector_GetNextCapacity() );

    alc.construct( &alc.ptr[alc.num_elements_used], t );
    ++alc.num_elements_used;
  }

  void push_back( Type &&t )
  {
    if( alc.num_elements_used == capacity() )
      vector_ReAllocate( vector_GetNextCapacity() );

    alc.construct( &alc.ptr[alc.num_elements_used], std::forward<Type>( t ) );
    ++alc.num_elements_used;
  }

  template<class... Args>
  Type &emplace_back( Args&&... args )
  {
    if( alc.num_elements_used == capacity() )
      vector_ReAllocate( vector_GetNextCapacity() );

    alc.construct( &alc.ptr[alc.num_elements_used], std::forward<Args>( args )... );
    ++alc.num_elements_used;

    return alc.ptr[alc.num_elements_used - 1];
  }
};

// specialization for pointer types
template<class Type, typename Allocator> class MVKVectorImpl<Type*, Allocator> : public MVKVector<Type*>
{
  friend class MVKVectorImpl;

  Allocator  alc;

public:
  class iterator
  {
    MVKVectorImpl *vector;
    size_t         index;

  public:
    iterator() = delete;
    iterator( const size_t _index, MVKVectorImpl &_vector ) : vector{ &_vector }, index{ _index } { }

    iterator &operator=( const iterator &it )
    {
      vector = it.vector;
      index = it.index;
      return *this;
    }

    Type *&operator*() { return vector->alc[index]; }

    bool operator==( const iterator &it ) const { return vector == it.vector && index == it.index; }
    bool operator!=( const iterator &it ) const { return vector != it.vector || index != it.index; }

    iterator& operator++() { ++index; return *this; }
    iterator  operator++( int ) { auto t = *this; ++index; return t; }

    bool   is_valid()     const { return index < vector->alc.size(); }
    size_t get_position() const { return index; }
  };

private:
  // this is the growth strategy -> adjust to your needs
  size_t vector_GetNextCapacity() const
  {
    constexpr auto ELEMENTS_FOR_64_BYTES = 64 / sizeof( Type );
    constexpr auto MINIMUM_CAPACITY = ELEMENTS_FOR_64_BYTES > 4 ? ELEMENTS_FOR_64_BYTES : 4;
    const auto current_capacity = capacity();
    return MINIMUM_CAPACITY + ( 3 * current_capacity ) / 2;
  }

  void vector_Allocate( const size_t s )
  {
    const auto new_reserved_size = tm_max( s, size() );

    alc.allocate( new_reserved_size );
  }

  void vector_ReAllocate( const size_t s )
  {
    alc.re_allocate( s );
  }

public:
  MVKVectorImpl() : MVKVector<Type*>{ &alc }
  {
  }

  MVKVectorImpl( const size_t n, const Type *t ) : MVKVector<Type*>{ &alc }
  {
    if ( n > 0 )
    {
      alc.allocate( n );

      for ( size_t i = 0; i < n; ++i )
      {
        alc.ptr[i] = t;
      }

      alc.num_elements_used = n;
    }
  }

  MVKVectorImpl( const MVKVectorImpl &a ) : MVKVector<Type*>{ &alc }
  {
    const size_t n = a.size();

    if ( n > 0 )
    {
      alc.allocate( n );

      for ( size_t i = 0; i < n; ++i )
      {
        alc.ptr[i] = a.alc.ptr[i];
      }

      alc.num_elements_used = n;
    }
  }

  MVKVectorImpl( MVKVectorImpl &&a ) : MVKVector<Type*>{ &alc }, alc{ std::move( a.alc ) }
  {
  }

  MVKVectorImpl( std::initializer_list<Type*> vector ) : MVKVector<Type*>{ &alc }
  {
    if ( vector.size() > capacity() )
    {
      vector_Allocate( vector.size() );
    }

    // std::initializer_list does not yet support std::move, we use it anyway but it has no effect
    for ( auto element : vector )
    {
      alc.ptr[alc.num_elements_used] = element;
      ++alc.num_elements_used;
    }
  }

  ~MVKVectorImpl()
  {
  }

  template<typename U>
  MVKVectorImpl& operator=( const U &a )
  {
    static_assert( std::is_base_of<MVKVector<U>, U>::value, "argument is not of type MVKVector" );

    if ( this != reinterpret_cast< const MVKVector<Type>* >( &a ) )
    {
      const auto n = a.size();

      if ( alc.num_elements_used == n )
      {
        for ( size_t i = 0; i < n; ++i )
        {
          alc.ptr[i] = a.alc.ptr[i];
        }
      }
      else
      {
        if ( n > capacity() )
        {
          vector_ReAllocate( n );
        }

        for ( size_t i = 0; i < n; ++i )
        {
          alc.ptr[i] = a[i];
        }

        alc.num_elements_used = n;
      }
    }

    return *this;
  }

  MVKVectorImpl& operator=( MVKVectorImpl &&a )
  {
    alc.swap( a.alc );
    return *this;
  }

  bool operator==( const MVKVectorImpl &a ) const
  {
    if ( alc.num_elements_used != a.alc.num_elements_used )
      return false;
    for ( size_t i = 0; i < alc.num_elements_used; ++i )
    {
      if ( alc[i] != a.alc[i] )
        return false;
    }
    return true;
  }

  bool operator!=( const MVKVectorImpl &a ) const
  {
    if ( alc.num_elements_used != a.alc.num_elements_used )
      return true;
    for ( size_t i = 0; i < alc.num_elements_used; ++i )
    {
      if ( alc.ptr[i] != a.alc[i] )
        return true;
    }
    return false;
  }

  void swap( MVKVectorImpl &a )
  {
    alc.swap( a.alc );
  }

  void clear()
  {
    alc.num_elements_used = 0;
  }

  void reset()
  {
    alc.deallocate();
  }

  iterator begin()        { return iterator( 0, *this ); }
  iterator end()          { return iterator( alc.num_elements_used, *this ); }
  size_t   size()   const { return alc.num_elements_used; }
  bool     empty()  const { return alc.num_elements_used == 0; }

  Type *at( const size_t i ) const
  {
    return alc.ptr[i];
  }

  Type * const &operator[]( const size_t i ) const override
  {
    return alc.ptr[i];
  }

  Type *&operator[]( const size_t i )
  {
    return alc.ptr[i];
  }

  const Type * const *data() const
  {
    return &alc.ptr[0];
  }

  Type **data()
  {
    return &alc.ptr[0];
  }

  size_t capacity() const
  {
    return alc.get_capacity();
  }

  const Type *back() const
  {
    return alc.ptr[alc.num_elements_used - 1];
  }

  Type *back()
  {
    return alc.ptr[alc.num_elements_used - 1];
  }

  void pop_back()
  {
    if ( alc.num_elements_used > 0 )
    {
      --alc.num_elements_used;
    }
  }

  void reserve( const size_t new_size )
  {
    if ( new_size > capacity() )
    {
      vector_ReAllocate( new_size );
    }
  }

  void assign( const size_t new_size, const Type *t )
  {
    if ( new_size <= capacity() )
    {
      clear();
    }
    else
    {
      vector_Allocate( new_size );
    }

    for ( size_t i = 0; i < new_size; ++i )
    {
      alc.ptr[i] = const_cast< Type* >( t );
    }

    alc.num_elements_used = new_size;
  }

  void resize( const size_t new_size )
  {
    if ( new_size == alc.num_elements_used )
    {
      return;
    }

    if ( new_size == 0 )
    {
      clear();
      return;
    }

    if ( new_size > alc.num_elements_used )
    {
      if ( new_size > capacity() )
      {
        vector_ReAllocate( new_size );
      }

      while ( alc.num_elements_used < new_size )
      {
        alc.ptr[alc.num_elements_used] = nullptr;
        ++alc.num_elements_used;
      }
    }
    else
    {
      alc.num_elements_used = new_size;
    }
  }

  // trims the capacity of the MVKVector to the number of used elements
  void shrink_to_fit()
  {
    alc.shrink_to_fit();
  }

  void erase( const iterator it )
  {
    if ( it.is_valid() )
    {
      --alc.num_elements_used;

      for ( size_t i = it.get_position(); i < alc.num_elements_used; ++i )
      {
        alc.ptr[i] = alc.ptr[i + 1];
      }
    }
  }

  // adds t before position it and automatically resizes vector if necessary
  void insert( const iterator it, const Type *t )
  {
    if ( !it.is_valid() || alc.num_elements_used == 0 )
    {
      push_back( t );
    }
    else
    {
      if ( alc.num_elements_used == capacity() )
        vector_ReAllocate( vector_GetNextCapacity() );

      // move the remaining elements
      const size_t it_position = it.get_position();
      for ( size_t i = alc.num_elements_used; i > it_position; --i )
      {
        alc.ptr[i] = alc.ptr[i - 1];
      }

      alc.ptr[it_position] = const_cast< Type* >( t );
      ++alc.num_elements_used;
    }
  }

  void push_back( const Type *t ) override
  {
    if ( alc.num_elements_used == capacity() )
      vector_ReAllocate( vector_GetNextCapacity() );

    alc.ptr[alc.num_elements_used] = const_cast< Type* >( t );
    ++alc.num_elements_used;
  }
};


template<typename Type>
using MVKVectorDefault = MVKVectorImpl<Type, mvk_vector_allocator_default<Type>>;

template<typename Type, size_t N = 8>
using MVKVectorInline  = MVKVectorImpl<Type, mvk_vector_allocator_with_stack<Type, N>>;


#endif


