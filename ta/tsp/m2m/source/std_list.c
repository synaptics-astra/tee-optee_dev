/*
 * INTERNAL USE ONLY
 *
 * Unpublished Work Copyright (C) 2013-2017 Synaptics Incorporated.
 * All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 */

///////////////////////////////////////////////////////////////////////////////
//! \file		std_list.c
//! \brief
//! \author		Cheng Du
//! \version	0.1
//! \date		July 2007
//! \Histroy	4/23/2008	Fang Bao	Add std_list_search()
///////////////////////////////////////////////////////////////////////////////
#include "m2m_ta_comm.h"
#include "std_list.h"

#ifdef TA_COMPILE
#define E_SUC                           ( 0x00000000 )
#define E_ERR                           ( 0x80000000 )

#define E_STDLIST_BASE                  ( 0x0000 << 16 )

#define E_STDLIST_SUC( code )           ( E_SUC | E_STDLIST_BASE | ( code & 0x0000FFFF ) )
#define E_STDLIST_ERR( code )           ( E_ERR | E_STDLIST_BASE | ( code & 0x0000FFFF ) )

#define E_STDLIST_NOTIMPL               E_STDLIST_ERR( 0x4001 )	// Not implemented
#define E_STDLIST_NOINTERFACE           E_STDLIST_ERR( 0x4002 )	// No such interface supported
#define E_STDLIST_POINTER               E_STDLIST_ERR( 0x4003 )	// Pointer that is not valid
#define E_STDLIST_ABORT                 E_STDLIST_ERR( 0x4004 )	// Operation aborted
#define E_STDLIST_FAIL                  E_STDLIST_ERR( 0x4005 )	// Unspecified failure
#define E_STDLIST_OUTOFMEMORY           E_STDLIST_ERR( 0x700E )	// Failed to allocate necessary memory

#define GaloisFree   dmx_free
#define GaloisMalloc dmx_malloc
#define FAILED(hr)			((HRESULT)(hr) < 0)
#define CHKHR			{ if (FAILED(hr)) { return hr; }}
#endif

std_list_alloc_func g_std_list_malloc = NULL;

HRESULT CreateNode(std_list* self, PVOID pData, std_list_node** ppNode);

std_list* std_list_ctor(std_list* self, std_list_data_dtor_func data_dtor)
{
	if (!self)
	{
		if (g_std_list_malloc)
		{
			self = (std_list*) g_std_list_malloc( sizeof(std_list) );
		}
		else
		{
			self = (std_list*) GaloisMalloc( sizeof(std_list) );
		}
	}

	if (self)
	{
		self->m_pHead = NULL;
		self->m_pTail = NULL;
		self->m_uiSize = 0;
		self->m_data_dtor = data_dtor;
	}

	return self;
}

VOID std_list_dtor(std_list* self)
{
	if (self)
	{
		std_list_clear(self);
		if (!g_std_list_malloc)
		{
			GaloisFree(self);
		}
	}
}

HRESULT std_list_clear(std_list* self)
{
	std_list_node *pNode, *pNext;

	if (!self) return E_STDLIST_POINTER;
	if (self->m_uiSize == 0) return S_OK;

	for (pNode = self->m_pHead; pNode != NULL; )
	{
		pNext = pNode->pNext;
		if (self->m_data_dtor) self->m_data_dtor(pNode->pData);
		if (!g_std_list_malloc)
		{
			GaloisFree(pNode);
		}
		pNode = pNext;
	}

	self->m_pHead = NULL;
	self->m_pTail = NULL;
	self->m_uiSize = 0;
	self->m_data_dtor = NULL;

	return S_OK;
}

HRESULT std_list_pop_back(std_list* self, PVOID* ppData)
{
	std_list_node* pNode = NULL;

	if (!self || !ppData) return E_STDLIST_POINTER;

	if (self->m_uiSize == 0)
	{
		*ppData = NULL;
		return E_FAIL;
	}
	else
	{
		// get data
		*ppData = self->m_pTail->pData;

		// save the old tail and head
		pNode = self->m_pTail;

		// set the new tail
		self->m_pTail = pNode->pPrev;
		if (self->m_pTail)
		{
			self->m_pTail->pNext = NULL;
		}
		else
		{
			self->m_pHead = NULL;
		}

		// release the old tail
		if (!g_std_list_malloc)
		{
			GaloisFree(pNode);
		}

		// update size
		self->m_uiSize--;

		return S_OK;
	}
}

HRESULT std_list_pop_front(std_list* self, PVOID* ppData)
{
	std_list_node* pNode = NULL;

	if (!self || !ppData) return E_STDLIST_POINTER;

	if (self->m_uiSize == 0)
	{
		*ppData = NULL;
		return E_FAIL;
	}
	else
	{
		// get data
		*ppData = self->m_pHead->pData;

		// save the old head
		pNode = self->m_pHead;

		// set the new head and tail
		self->m_pHead = self->m_pHead->pNext;
		if (self->m_pHead)
		{
			self->m_pHead->pPrev = NULL;
		}
		else
		{
			self->m_pTail = NULL;
		}

		// release the old head
		if (!g_std_list_malloc)
		{
			GaloisFree(pNode);
		}

		// update size
		self->m_uiSize--;

		return S_OK;
	}
}

HRESULT std_list_push_back(std_list* self, PVOID pData)
{
	HRESULT hr = S_OK;
	std_list_node* pNode = NULL;

	if (!self) return E_STDLIST_POINTER;

	hr = CreateNode(self, pData, &pNode); CHKHR;

	// append node
	if (self->m_uiSize == 0) // no node
	{
		self->m_pHead = pNode;
	}
	else
	{
		self->m_pTail->pNext = pNode;
		pNode->pPrev = self->m_pTail;
	}

	self->m_pTail = pNode;

	// update size
	self->m_uiSize++;

	return hr;
}

HRESULT std_list_push_front(std_list* self, PVOID pData)
{
	HRESULT hr = S_OK;
	std_list_node* pNode = NULL;

	if (!self) return E_STDLIST_POINTER;

	hr = CreateNode(self, pData, &pNode); CHKHR;

	// insert node
	if (self->m_uiSize == 0) // no node
	{
		self->m_pTail = pNode;
	}
	else
	{
		pNode->pNext = self->m_pHead;
		self->m_pHead->pPrev = pNode;
	}

	self->m_pHead = pNode;

	// update size
	self->m_uiSize++;

	return hr;
}

HRESULT std_list_insert(std_list* self, std_list_node* pNode, PVOID pData)
{
	HRESULT hr = S_OK;
	std_list_node* pNewNode = NULL;
	std_list_node* pNextNode = NULL;

	if (!self || !pNode) return E_STDLIST_POINTER;
	if (self->m_uiSize == 0) return E_FAIL;

	hr = CreateNode(self, pData, &pNewNode); CHKHR;

	pNextNode = pNode->pNext;

	// update the current node (i.e. insert the new node AFTER the current node)
	pNode->pNext = pNewNode;

	// update the new node
	pNewNode->pPrev = pNode;
	pNewNode->pNext = pNextNode;

	// update the next node
	if (pNextNode)
	{
		pNextNode->pPrev = pNewNode;
	}

	// update tail
	if (self->m_pTail == pNode)
	{
		self->m_pTail = pNewNode;
	}

	// update size
	self->m_uiSize++;

	return hr;
}

HRESULT std_list_remove(std_list* self, std_list_node* pNode)
{
	std_list_node* pPrevNode = NULL;
	std_list_node* pNextNode = NULL;

	if (!self || !pNode) return E_STDLIST_POINTER;
	if (self->m_uiSize == 0) return E_FAIL;

	pPrevNode = pNode->pPrev;
	pNextNode = pNode->pNext;

	// update previous node
	if (pPrevNode)
	{
		pPrevNode->pNext = pNextNode;
	}

	// update next node
	if (pNextNode)
	{
		pNextNode->pPrev = pPrevNode;
	}

	// update head
	if (self->m_pHead == pNode)
	{
		self->m_pHead = pNextNode;
	}

	// update tail
	if (self->m_pTail == pNode)
	{
		self->m_pTail = pPrevNode;
	}

	// update size
	self->m_uiSize--;

	// destruct the data
	if (self->m_data_dtor)
	{
		self->m_data_dtor(pNode->pData);
	}

	// destruct the node
	if (!g_std_list_malloc)
	{
		GaloisFree(pNode);
	}

	return S_OK;
}

HRESULT std_list_size(std_list* self, UINT* puiSize)
{
	if (!self || !puiSize) return E_STDLIST_POINTER;

	*puiSize = self->m_uiSize;

	return S_OK;
}

HRESULT std_list_iter(std_list* self, std_list_node** ppNode)
{
	if (!self || !ppNode) return E_STDLIST_POINTER;

	if(*ppNode == 0)
	{
		*ppNode = self->m_pHead;
	}
	else
	{
		*ppNode = (*ppNode)->pNext;
	}

	return S_OK;
}

HRESULT std_list_first(std_list* self, std_list_node** ppNode)
{
	if (!self || !ppNode) return E_STDLIST_POINTER;

	*ppNode = self->m_pHead;

	return S_OK;
}

HRESULT std_list_last(std_list* self, std_list_node** ppNode)
{
	if (!self || !ppNode) return E_STDLIST_POINTER;

	*ppNode = self->m_pTail;

	return S_OK;
}

HRESULT std_list_front(std_list* self, PVOID* ppData)
{
	if (!self || !ppData) return E_STDLIST_POINTER;

	if (self->m_pHead)
	{
		*ppData = self->m_pHead->pData;
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT std_list_back(std_list* self, PVOID* ppData)
{
	if (!self || !ppData) return E_STDLIST_POINTER;

	if (self->m_pTail)
	{
		*ppData = self->m_pTail->pData;
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

///////////////////////////////////////////////////////////////////////////////

HRESULT CreateNode(std_list* self, PVOID pData, std_list_node** ppNode)
{
	std_list_node* pNode = NULL;

	if (!self || !ppNode) return E_STDLIST_POINTER;

	if (g_std_list_malloc)
	{
		pNode = (std_list_node*) g_std_list_malloc( sizeof(std_list_node) );
	}
	else
	{
		pNode = (std_list_node*) GaloisMalloc( sizeof(std_list_node) );
	}

	if (!pNode) return E_STDLIST_OUTOFMEMORY;

	pNode->pData = pData;
	pNode->pPrev = NULL;
	pNode->pNext = NULL;

	*ppNode = pNode;

	return S_OK;
}

std_list_node* std_list_search(std_list* self, PVOID pData)
{
	std_list_node* pNode;

	if (std_list_first(self, &pNode) != S_OK)
		return NULL;

	for (; pNode != NULL;)
	{
		if (pNode->pData == pData)
			break;

		if (std_list_iter(self, &pNode) != S_OK)
			return NULL;
	}

	return pNode;
}
