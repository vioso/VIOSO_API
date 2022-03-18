#ifndef VWB_SPTR_INCLUDE_HPP
#define VWB_SPTR_INCLUDE_HPP

/*! \brief A class to smart harness ordinary pointers
 *
 *  Use this class for automatic use of pointers.
 *  Pointers are automatically deleted if not used anymore.
 *  SPtr keeps an internal reference counter.
 */

#ifndef ASSERT
#define ASSERT(exp) (NULL)
#endif

template< class T >
class SPtr
{
public:
	volatile unsigned int* piRef;		//< the reference counter
	volatile unsigned int* piWRef;		//< the waek reference counter
	T* ptr;				//< the object pointer

	/**
	* Default Constructor
	* constructs an empty object
	*/
	SPtr()
	: ptr( NULL )
	, piRef( NULL )
	, piWRef( NULL )
	{
	}

	/**
	* Construct an object from another smartpointer of other template type, which must be a subclass
	* @param p pointer to a valid pointer of the template type, it always will be attached
	* do not delete original pointer afterwards! 
	*/
	template<class T2>
	SPtr( SPtr<T2> const& other )
	: ptr( dynamic_cast< T* >( other.ptr ) )
	, piRef( other.piRef )
	, piWRef( other.piWRef )
	{
		if( NULL == ptr )
		{
			piRef = NULL;
			piWRef = NULL;
		}
		else if( piRef )
		{
			InterlockedIncrement( piRef );
		}
	}

	/**
	* Copy constructor, actually a refinement of the above but necessary to overwrite implicit copy constructor
	* Construct an object from another object of the same type
	* @param other the object to make a copy from
	*/
	SPtr( SPtr const& other )
	: piRef( ((unsigned int*)(intptr_t)( other.piRef ? InterlockedIncrement( other.piRef ) : 0 ), other.piRef) )
	, ptr( other.ptr )
	, piWRef( other.piRef )
	{
		ASSERT( ( NULL == ptr && NULL == piRef ) || ( ptr && piRef ) );
	}

	/**
	* Construct an object from a value of the template type
	* @param v const reference to a value
	*/
	explicit SPtr( T const& v )
	: ptr( new T )
	, piRef( new volatile unsigned int )
	, piWRef( new volatile unsigned int )
	{
		*piRef = 1;
		*piWRef = 0;
		*ptr = v;
	}

	/**
	* Construct an object from a pointer of the template type
	* @param p pointer to a valid pointer of the template type, it always will be attached
	* do not delete original pointer afterwards! 
	*/
	SPtr( T* p )
	: ptr( p )
	, piRef( p ? new volatile unsigned int : NULL )
	, piWRef( p ? new volatile unsigned int : NULL )
	{
		if( piRef )
			*piRef = 1;
		if( piWRef )
			*piRef = 0;
	}

	unsigned int ResetDirty()
	{
		if( piRef &&
			0 == ::InterlockedDecrement( piRef ) )
		{
			if( 0 == piWRef )
			{
				delete piWRef;
				delete piRef;
			}
			if( ptr )
				delete ptr;
		}
		return piRef ? *piRef : 0;
	}

	unsigned int Reset( T* v = NULL )
	{
		ResetDirty();
		ptr = v;
		if( ptr )
		{
			piRef = new volatile unsigned int;
			*piRef = 1;
			piWRef = new volatile unsigned int;
			*piWRef = 0;
		}
		else
		{
			piRef = NULL;
			piWRef = 0;
		}
		return piRef ? *piRef : 0;
	}

	/**
	* Destructor
	*/
	~SPtr()
	{ 
		ResetDirty();
	}

	/**
	* operator ==
	* @param other smartpointer to compare me to
	* @return true if internal pointers are equal, thus pointing to the same literal
	*/
	bool operator ==( SPtr const& other ) const { return ptr == other.ptr; }

	/**
	* operator !=
	* @param other smartpointer to compare me to
	* @return true if internal pointers are not equal, thus not pointing to the same literal
	*/
	bool operator !=( SPtr const& other ) const { return ptr != other.ptr; }

	/**
	* operator bool
	* @return true if internal pointer valid
	*/
	operator bool() const { return ptr && piRef && 0 != *piRef; }

	
	/**
	* operator =
	* Standard assignment operator
	* @param other const reference of another smartpointer
	* @return reference of this object
	*/
	SPtr& operator=( SPtr const& other ) 
	{ 
		if( other.piRef )
			InterlockedIncrement( other.piRef );

		ResetDirty();

		ptr = other.ptr;
		piRef = other.piRef;
		piWRef = other.piWRef;
		return *this;
	}

	/**
	* operator =
	* assign some pointer of my type to be managed
	* the currently assiged pointer will be released
	* @param p some pointer of my type
	* @return reference of this object
	*/
	SPtr& operator=( T* p ) 
	{ 
		if( ptr == p  )
			return *this;

		Reset( p );
		return *this;
	}

	/**
	* operator ->
	* operator to dereference the smartpointer like a pointer
	*/
	T* operator->() { return ptr; }

	/**
	* operator -> const
	* operator to dereference the smartpointer like a pointer
	*/
	T const* operator->() const { return ptr; }

	/**
	* operator &
	* operator to make & references to the containing pointer
	* this is used to assign a new value to the containing pointer.
	* This is used for all kinds of create routines.
	* It will ASSERT if the currently assigned pointer is not NULL, as it will probably
	* be overwritten and the former pointer gets lost.
	* Use &var.ptr instead, if you intend to use a not const reference.
	* @return a reference of the managed COM pointer
	*/
	T** operator&() 
	{ 
		ASSERT( NULL == piRef && NULL == ptr );
		piRef = new volatile unsigned int;
		*piRef = 1;
		piWRef = new volatile unsigned int;
		*piWRef = 0;
		return &ptr; 
	}

	/**
	* operator * const
	* dereferences the internal pointer
	*/
	T const& operator*() const { return *ptr; }

	/**
	* operator *
	* dereferences the internal pointer
	*/
	T& operator*() { return *ptr; }

	/**
	* Create
	* creates an empty smartpointer object
	* @return reference to this object
	*/
	SPtr& Create()
	{
		ResetDirty();
		piRef = new volatile unsigned int;
		*piRef = 1;
		piWRef = new volatile unsigned int;
		*piWRef = 0;
		ptr = new T;
	}

	/**
	* Create
	* creates an smartpointer object
	* @param a value to the managed type, this will be used as parameter in the type's constructor
	* @return reference to this object
	*/
	template< class... P >
	SPtr& Create( const P&... args )
	{
		ResetDirty();
		piRef = new volatile unsigned int;
		*piRef = 1;
		piWRef = new volatile unsigned int;
		*piWRef = 0;
		ptr = new T( args... );
	}

	/** ref
	* get the current reference count of the managed pointer
	* @return current reference count of the managed pointer
	*/
	unsigned int ref() const { return piRef ? *piRef : 0; }

	/*
	* attach
	* attaches a pointer of my type to be managed
	* @param p pointer of my type, do not delete the given pointer afterwards
	* @return a reference to this object
	*/
	SPtr& attach( T* p )
	{ 
		Reset(p);
		return *this; 
	}

	/*
	* detach
	* detaches the pointer from the object
	* The object will become empty after detaching.
	* This must be the only object managing this pointer, thus the reference counter has to be 1.
	* @return the pointer formally managed by the object
	*/
	T* detach() 
	{ 
		T* ret = ptr;
		ptr = NULL;
		Reset();
		return ret;
	}
};

template< class T >
class WPtr
{
protected:
	SPtr<T> ref;
public:
	WPtr( SPtr<T> const& p )
	{
		if( p.piWRef )
			::InterlockedIncrement( p.piWRef );
		ref.piRef = p.piRef;
		ref.piWRef = p.piWRef;
		ref.ptr = p.ptr;
	}
	~WPtr()
	{
		if( ref.piWRef && 0 == ::InterlockedDecrement( ref.piWRef ) )
		{
			delete ref.piWef;
			delete ref.piWRef;
		}
	}

	operator SPtr<T>() { return *ref.piRef ? SPtr<T>( ref ) : SPtr<T>(); };

	operator T() { return *ref.piRef ? SPtr<T>( ref ).operator T() : 0; }
};

#endif //ndef VWB_SPTR_INCLUDE_HPP


