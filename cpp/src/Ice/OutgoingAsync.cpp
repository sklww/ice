// **********************************************************************
//
// Copyright (c) 2003
// ZeroC, Inc.
// Billerica, MA, USA
//
// All Rights Reserved.
//
// Ice is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License version 2 as published by
// the Free Software Foundation.
//
// **********************************************************************

#include <Ice/OutgoingAsync.h>
#include <Ice/Object.h>
#include <Ice/Connection.h>
#include <Ice/Reference.h>
#include <Ice/Instance.h>
#include <Ice/LocalException.h>
#include <Ice/Properties.h>
#include <Ice/LoggerUtil.h>
#include <Ice/LocatorInfo.h>

using namespace std;
using namespace Ice;
using namespace IceInternal;

void IceInternal::incRef(OutgoingAsync* p) { p->__incRef(); }
void IceInternal::decRef(OutgoingAsync* p) { p->__decRef(); }

void IceInternal::incRef(AMI_Object_ice_invoke* p) { p->__incRef(); }
void IceInternal::decRef(AMI_Object_ice_invoke* p) { p->__decRef(); }

IceInternal::OutgoingAsync::OutgoingAsync() :
    __is(0),
    __os(0),
    __cnt(0)
{
}

IceInternal::OutgoingAsync::~OutgoingAsync()
{
    delete __is;
    delete __os;

    if(_connection)
    {
	_connection->decProxyCount();
    }
}

void
IceInternal::OutgoingAsync::__setup(const ReferencePtr& ref)
{
    const_cast<ReferencePtr&>(_reference) = ref;
}

void
IceInternal::OutgoingAsync::__finished(BasicStream& is)
{
    DispatchStatus status;

    try
    {
	__is->swap(is);
	Byte b;
	__is->read(b);
	status = static_cast<DispatchStatus>(b);

	switch(status)
	{
	    case DispatchOK:
	    case DispatchUserException:
	    {
		__is->startReadEncaps();
		break;
	    }
	    
	    case DispatchObjectNotExist:
	    {
		ObjectNotExistException ex(__FILE__, __LINE__);
		ex.id.__read(__is);
		__is->read(ex.facet);
		__is->read(ex.operation);
		throw ex;
	    }

	    case DispatchFacetNotExist:
	    {
		FacetNotExistException ex(__FILE__, __LINE__);
		ex.id.__read(__is);
		__is->read(ex.facet);
		__is->read(ex.operation);
		throw ex;
	    }
	    
	    case DispatchOperationNotExist:
	    {
		OperationNotExistException ex(__FILE__, __LINE__);
		ex.id.__read(__is);
		__is->read(ex.facet);
		__is->read(ex.operation);
		throw ex;
	    }
	    
	    case DispatchUnknownException:
	    {
		UnknownException ex(__FILE__, __LINE__);
		__is->read(ex.unknown);
		throw ex;
	    }
	    
	    case DispatchUnknownLocalException:
	    {
		UnknownLocalException ex(__FILE__, __LINE__);
		__is->read(ex.unknown);
		throw ex;
	    }
	    
	    case DispatchUnknownUserException:
	    {
		UnknownUserException ex(__FILE__, __LINE__);
		__is->read(ex.unknown);
		throw ex;
	    }
	    
	    default:
	    {
		UnknownReplyStatusException ex(__FILE__, __LINE__);
		throw ex;
	    }
	}
    }
    catch(const LocalException& ex)
    {
	__finished(ex);
	return;
    }

    assert(status == DispatchOK || status == DispatchUserException);

    try
    {
	__response(status == DispatchOK);
    }
    catch(const Exception& ex)
    {
	warning(ex);
    }
    catch(const std::exception& ex)
    {
	warning(ex);
    }
    catch(...)
    {
	warning();
    }
}

void
IceInternal::OutgoingAsync::__finished(const LocalException& exc)
{
    if(_reference->locatorInfo)
    {
	_reference->locatorInfo->clearObjectCache(_reference);
    }

/*
    ProxyFactoryPtr proxyFactory = _reference->instance->proxyFactory();
    if(proxyFactory)
    {
	proxyFactory->checkRetryAfterException(ex, cnt);
    }
    else
    {
        ex.ice_throw(); // The communicator is already destroyed, so we cannot retry.
    }
*/

    try
    {
	ice_exception(exc);
    }
    catch(const Exception& ex)
    {
	warning(ex);
    }
    catch(const std::exception& ex)
    {
	warning(ex);
    }
    catch(...)
    {
	warning();
    }
}

bool
IceInternal::OutgoingAsync::__timedOut() const
{
    if(_connection->timeout() >= 0)
    {
	return IceUtil::Time::now() >= _absoluteTimeout;
    }
    else
    {
	return false;
    }
}

void
IceInternal::OutgoingAsync::__prepare(const string& operation, OperationMode mode, const Context& context)
{
    delete __is;
    delete __os;
    __is = new BasicStream(_reference->instance.get());
    __os = new BasicStream(_reference->instance.get());

    if(_connection)
    {
	_connection->decProxyCount();
	_connection = 0;
    }

    _connection = _reference->getConnection();
    _connection->incProxyCount();

    _connection->prepareRequest(__os);
    _reference->identity.__write(__os);
    __os->write(_reference->facet);
    __os->write(operation);
    __os->write(static_cast<Byte>(mode));
    __os->writeSize(Int(context.size()));
    Context::const_iterator p;
    for(p = context.begin(); p != context.end(); ++p)
    {
	__os->write(p->first);
	__os->write(p->second);
    }

    __os->startWriteEncaps();
}

void
IceInternal::OutgoingAsync::__send()
{
    try
    {
	_connection->sendAsyncRequest(this);
    }
    catch(const LocalException&)
    {
	//
	// Twoway requests report exceptions using finished().
	//
	assert(false);
    }

    if(_connection->timeout() >= 0)
    {
	_absoluteTimeout = IceUtil::Time::now() + IceUtil::Time::milliSeconds(_connection->timeout());
    }
}

void
IceInternal::OutgoingAsync::warning(const Exception& ex) const
{
    if(_reference->instance->properties()->getPropertyAsIntWithDefault("Ice.Warn.AMICallback", 1) > 0)
    {
	Warning out(_reference->instance->logger());
	out << "Ice::Exception raised by AMI callback:\n" << ex;
    }
}

void
IceInternal::OutgoingAsync::warning(const std::exception& ex) const
{
    if(_reference->instance->properties()->getPropertyAsIntWithDefault("Ice.Warn.AMICallback", 1) > 0)
    {
	Warning out(_reference->instance->logger());
	out << "std::exception raised by AMI callback:\n" << ex.what();
    }
}

void
IceInternal::OutgoingAsync::warning() const
{
    if(_reference->instance->properties()->getPropertyAsIntWithDefault("Ice.Warn.AMICallback", 1) > 0)
    {
	Warning out(_reference->instance->logger());
	out << "unknown exception raised by AMI callback";
    }
}

void
Ice::AMI_Object_ice_invoke::__invoke(const string& operation, OperationMode mode, const vector<Byte>& inParams,
				     const Context& context)
{
    try
    {
	__prepare(operation, mode, context);
	__os->writeBlob(inParams);
	__os->endWriteEncaps();
    }
    catch(const LocalException& ex)
    {
	__finished(ex);
	return;
    }
    __send();
}

void
Ice::AMI_Object_ice_invoke::__response(bool ok) // ok == true means no user exception.
{
    vector<Byte> outParams;
    try
    {
	Int sz = __is->getReadEncapsSize();
	__is->readBlob(outParams, sz);
    }
    catch(const LocalException& ex)
    {
	__finished(ex);
	return;
    }
    ice_response(ok, outParams);
}
