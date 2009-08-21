// **********************************************************************
//
// Copyright (c) 2003-2009 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

package IceInternal;

public class Ex
{
    public static void throwUOE(String expectedType, String actualType)
    {
        throw new Ice.UnexpectedObjectException(
                    "expected element of type `" + expectedType + "' but received '" + actualType,
                    actualType, expectedType);
    }

    public static void throwMemoryLimitException(int requested, int maximum)
    {
        throw new Ice.MemoryLimitException("requested " + requested + " bytes, maximum allowed is " + maximum +
                                           " bytes (see Ice.MessageSizeMax)");
    }

    //
    // A small utility to get the strack trace of the exception (which also includes toString()).
    //
    public static String toString(java.lang.Throwable ex)
    {        
        java.io.StringWriter sw = new java.io.StringWriter();
        java.io.PrintWriter pw = new java.io.PrintWriter(sw);
        ex.printStackTrace(pw);
        pw.flush();
        return sw.toString();
    }
}
