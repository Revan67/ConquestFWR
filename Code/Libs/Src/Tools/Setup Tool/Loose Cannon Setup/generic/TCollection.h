// TCollection.h
//
//
//
//


#ifndef TCOLLECTION_H
#define TCOLLECTION_H


template <class Type>
struct TCOLLECTIONNODE
{
	Type *item;
	TCOLLECTIONNODE *prev;
	TCOLLECTIONNODE *next;
	
	TCOLLECTIONNODE( BOOL copy, Type *i, TCOLLECTIONNODE *p=NULL, TCOLLECTIONNODE *n=NULL )
	{
		item = i;
		if( copy ) {
//			assert(0);
//			item = new Type;
//			if( i ) {
//				*item = *i;
//			}
		}
		if( prev=p ) {
			prev->next = this;
		}
		if( next=n ) {
			next->prev = this;
		}
	}

	~TCOLLECTIONNODE()
	{
		if( next ) {
			next->prev = prev;
		}
		if( prev ) {
			prev->next = next;
		}
	}
};

template <class Type>
class TCollection
{
public:
/*
	HRESULT AllocAtHead( Type **item )
	{
		m_Head = new TCOLLECTIONNODE<Type>(TRUE,NULL,NULL,m_Head);
		*item = m_Head->item;
		return S_OK;
	}

	HRESULT AllocAtTail( Type **item )
	{
		m_Tail = new TCOLLECTIONNODE<Type>(TRUE,NULL,m_Tail,NULL);
		*item = m_Tail->item;
		return S_OK;
	}
*/
	HRESULT AddToHead( Type *item )
	{
		m_Head = new TCOLLECTIONNODE<Type>(m_bOwner,item,NULL,m_Head);
		if( !m_Tail ) {
			m_Tail = m_Head;
		}
		return S_OK;
	}

	HRESULT AddToTail( Type *item )
	{
		m_Tail = new TCOLLECTIONNODE<Type>(m_bOwner,item,m_Tail,NULL);
		if( !m_Head) {
			m_Head = m_Tail;
		}
		return S_OK;
	}

	HRESULT FindItem( Type *item, TCOLLECTIONNODE<Type> **node=NULL )
	{
		for( TCOLLECTIONNODE<Type> *I=m_Head; I;I=I->next ) {
			if( I->item == item ) {
				if( node ) {
					*node = I;
				}
				return S_OK;
			}
		}
		return E_FAIL;
	}

	HRESULT Remove( Type *item )
	{
		if( m_bOwner ) {
			return Delete( item );			
		}
		return Unlink( item );			
	}

	HRESULT RemoveAllItems( void )
	{
		if( m_bOwner ) {
			return DeleteAllItems( );			
		}
		return UnlinkAllItems( );			
	}

	HRESULT Unlink( Type *item )
	{
		for( TCOLLECTIONNODE<Type> *n=m_Head; n; n=n->next ) {
			if( n->item == item ) {
				if( n == m_Head ) {
					m_Head = n->next;
				}
				if( n == m_Tail ) {
					m_Tail = n->prev;
				}
				delete n;
				
				return S_OK;
			}	
		}
		return E_FAIL;
	}

	HRESULT UnlinkAllItems( void )
	{
		TCOLLECTIONNODE<Type> *t = m_Head, *n;
		while( t ) {
			n = t->next;
			delete t;
			t = n;
		}
		m_Head = m_Tail = NULL;
		return S_OK;
	}
	
	HRESULT Delete( Type *item )
	{
		for( TCOLLECTIONNODE<Type> *n=m_Head; n; n=n->next ) {
			if( n->item == item ) {
				if( n == m_Head ) {
					m_Head = n->next;
				}
				if( n == m_Tail ) {
					m_Tail = n->prev;
				}
//				delete n->item;
//				n->item = NULL;
				delete n;
				
				return S_OK;
			}	
		}
		return E_FAIL;
	}

	HRESULT DeleteAllItems( void )
	{
		TCOLLECTIONNODE<Type> *t = m_Head, *n;
		while( t ) {
			n = t->next;
//			delete t->item;
//			t->item = NULL;
			delete t;
			t = n;
		}
		m_Head = m_Tail = NULL;
		return S_OK;
	}

	
	HRESULT Head( TCOLLECTIONNODE<Type> **node)
	{
		*node = m_Head;
		return S_OK;
	}

	HRESULT Tail( TCOLLECTIONNODE<Type> **node )
	{
		*node = m_Tail;
		return S_OK;
	}


	HRESULT SetOwner( BOOL bOwner )
	{
		m_bOwner = bOwner;
		return S_OK;
	}

	TCollection( BOOL bOwner = TRUE )
	{
		m_Head = m_Tail = NULL;
		m_bOwner = bOwner;
	}

	~TCollection()
	{
		if( m_bOwner ) {
			DeleteAllItems();
		}
		else {
			RemoveAllItems();
		}
	}

protected:

	TCOLLECTIONNODE<Type> *m_Head, *m_Tail;
	BOOL m_bOwner;

};

#endif
