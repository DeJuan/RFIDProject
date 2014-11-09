/*
 * Copyright (c) 2008 ThingMagic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
package com.thingmagic;

/**
 * A MultiReadPlan is a list of read plans which are used in sequence,
 * with a relative time corresponing to their relative weights.
 */
public class MultiReadPlan extends ReadPlan
{
  ReadPlan[] plans;
  int totalWeight;

  public MultiReadPlan(ReadPlan[] plans)
  {
    super();
    this.plans = plans;
    this.totalWeight = 0;
    for (ReadPlan r : this.plans)
    {
      this.totalWeight += r.weight;
    }
  }

  public MultiReadPlan(ReadPlan[] plans, int weight)
  {
    super(weight);
    this.plans = plans;
    this.totalWeight = 0;
    for (ReadPlan r : this.plans)
    {
      this.totalWeight += r.weight;
    }
  }

  @Override
  public String toString()
  {
    StringBuilder sb = new StringBuilder();

    sb.append("MultiReadPlan:[");
    if (plans.length > 0)
    {
      sb.append(plans[0].toString());
      for (int i = 1; i < plans.length; i++)
      {
        sb.append(", ").append(plans[i].toString());
      }
    }
    sb.append("]");

    return sb.toString();
  }
}
