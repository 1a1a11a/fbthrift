package test.fixtures.module0;

import com.facebook.swift.codec.*;

public enum Enum
{
    ONE(1), TWO(2), THREE(3);

    private final int value;

    Enum(int value)
    {
        this.value = value;
    }

    @ThriftEnumValue
    public int getValue()
    {
        return value;
    }
}
