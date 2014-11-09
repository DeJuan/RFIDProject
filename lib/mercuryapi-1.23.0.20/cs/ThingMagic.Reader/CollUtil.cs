using System;
using System.Collections.Generic;
using System.Text;

namespace ThingMagic
{
    /// <summary>
    /// ICollection conversion routines
    /// </summary>
    public static class CollUtil
    {
        /// <summary>
        /// Convert ICollection to Array
        /// </summary>
        /// <typeparam name="T">Type of the contents in the array</typeparam>
        /// <param name="collection">Collection of items</param>
        /// <returns>Array of items</returns>
        public static T[] ToArray<T>(ICollection<T> collection)
        {
            if (null == collection)
                throw new ArgumentNullException("collection");

            if (collection.GetType().IsArray)
                return (T[])collection;

            T[] array = null;

            lock (collection)
            {
                array = new T[collection.Count];
                collection.CopyTo(array, 0);
            }

            return array;
        }

        /// <summary>
        /// Convert integer array to string
        /// </summary>
        /// <param name="intArray">The input integer array</param>
        /// <returns>The converted string</returns>
        public static string IntArrayToString(int[] intArray)
        {
            string[] stringArray = new string[intArray.Length];
            for (int i = 0; i < intArray.Length; i++)
                stringArray[i]= intArray[i].ToString();
          string result = string.Join(",", stringArray);
          return result;
        }
        
    }
}
