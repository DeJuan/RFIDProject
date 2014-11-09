/*
 * Copyright (c) 2009 ThingMagic, Inc.
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

using System;

namespace ThingMagic
{
    /// <summary>
    /// Class representing GPIO pin number and state
    /// </summary>
    public class GpioPin : Object
    {
        private int _id;
        private bool _high;
        private bool _output;

        /// <summary>
        /// Create a GpioPin object
        /// </summary>
        /// <param name="id">GPIO pin number</param>
        /// <param name="high">True for pin high, False for pin low</param>
        public GpioPin(int id, bool high)
        {
            _id = id;
            _high = high;
        }

        /// <summary>
        /// Create a GpioPin object with direction
        /// </summary>
        /// <param name="id">GPIO pin number</param>
        /// <param name="high">True for pin high, False for pin low</param>
        /// <param name="output">True for output pin, False for input pin</param>
        public GpioPin(int id, bool high, bool output)
        {
            _id = id;
            _high = high;
            _output = output;
        }

        /// <summary>
        /// GPIO pin number
        /// </summary>
        public int Id
        {
            get { return _id; }
        }
        /// <summary>
        /// GPIO state: True=high, False=low
        /// </summary>
        public bool High
        {
            get { return _high; }
        }

        /// <summary>
        /// GPIO direction: True = output, False = Input
        /// </summary>
        public bool Output
        {
            get { return _output; }
        }

        /// <summary>
        /// Determines whether the specified Object is equal to the current Object.
        /// </summary>
        /// <param name="obj">The Object to compare with the current Object.</param>
        /// <returns>true if the specified Object is equal to the current Object; otherwise, false.</returns>
        public override bool Equals(Object obj)
        {
            if (null == obj) return false;
            if (!(obj is GpioPin)) return false;
            GpioPin gp = (GpioPin)obj;
            return ((this.Id == gp.Id) && (this.High == gp.High));
        }
        /// <summary>
        /// Serves as a hash function for a particular type.
        /// </summary>
        /// <returns>A hash code for the current Object.</returns>
        public override int GetHashCode()
        {
            int x;
            x = Id * 2;
            if (High)
                x++;
            return x;
        }
        /// <summary>
        /// Returns a String that represents the current Object.
        /// </summary>
        /// <returns>A String that represents the current Object.</returns>
        public override string ToString()
        {
            return String.Format("{0}{1}", Id, High ? "H" : "L");
        }

    }
}
