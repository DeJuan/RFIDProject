using System;
using System.Collections;
using System.Text;

namespace ThingMagic.RFIDSearchLight
{
    class ConvertEPC
    {
        static bool _allowUnofficialTags = false;
        public static bool AllowUnofficialTags
        {
            get { return _allowUnofficialTags; }
            set { _allowUnofficialTags = value; }
        }

        /// <summary>
        /// A Map for digit value to character.
        /// </summary>
        private static string base36Chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        /// <summary>
        /// A Map for digit value to character.
        /// </summary>
        private static string base41Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./@*#";
        /// <summary>
        /// This is the "canonical" entry, the external user does not care how we do it.
        /// </summary>
        /// <param name="bytes"></param>
        /// <returns></returns>
        public static string TagBytesToUserString(byte[] tagBytes)
        {
            string result = null;

            result = OfficialID(tagBytes);
            if (null == result)
            {
                if (AllowUnofficialTags)
                {
                    result = UnofficialID(tagBytes);
                }
            }
            return result;

        }

        private static string OfficialID(byte[] bytes)
        {
            //if (matchesPrefix(bytes, sgtin96Item00Prefix))
            {
                return BytesToBase36String(bytes, 38);
            }
           // return null;
        }

        private static string UnofficialID(byte[] bytes)
        {
            return EpcToBase36String(bytes);
            //return BytesToBase41String(bytes);
        }

        /// <summary>
        /// Encode bytes into base36.  Lower 36 bits encode directly, remaining 60 are abbreviated.
        /// </summary>
        /// <param name="bytes">Bytes to encode.  Must be 96 bits (12 bytes).</param>
        /// <returns>base36 string representing input bytes.</returns>
        private static string EpcToBase36String(byte[] bytes)
        {
            /*
             * Given a 96-bit EPC,
             * 
             * Check prefix
             * 
             *   If SGTIN-96 with ToolLink's EAN.UCC Company Prefix and Item Reference=00
             *     Encode last 38 bits to base36 then append '-'
             * 
             *   If first 60-bits match 01TMTOOLLINK or 0802AUTOSHOW prefixes,
             *     Encode last 36 bits to base36 then append ':'
             * 
             *   Else
             *     Encode all bits to base41 (3 base41 digits / 16 bits)
             */

             if (matchesPrefix(bytes, sgtin96Item00Prefix))
             {
                 return BytesToBase36String(bytes, 38) + '-';
             }
             else
             //else if (matchesPrefix(bytes, Gid01tmtoollinkPrefix) ||
             //         matchesPrefix(bytes, Gid0802autoshowPrefix))
             {
                 return BytesToBase36String(bytes, 36) + ':';
             }
             //else
             //{
             //    return BytesToBase41String(bytes);
             //}
         }


        // Prefixes //////////////////////////////////////////////////////////

        /// <summary>
        /// Prefix to compare against EPC [most-significant bits]
        /// </summary>
        private class Prefix
        {
            public byte[] value;
            public byte[] mask;

            public Prefix(byte[] value, byte[] mask)
            {
                this.value = value;
                this.mask = mask;
            }
        }
        private static Prefix sgtin96Item00Prefix = new Prefix(
            new byte[] { 0x30, 0x28, 0x35, 0x4D, 0x82, 0x02, 0x00, 0x36 },
            new byte[] { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0 });
        private static Prefix Gid01tmtoollinkPrefix = new Prefix(
            new byte[] { 0x01, 0x7A, 0xE7, 0xFA, 0xF7, 0x9A, 0x6B, 0x00 },
            new byte[] { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0 });
        private static Prefix Gid0802autoshowPrefix = new Prefix(
            new byte[] { 0x06, 0x7E, 0xFF, 0x58, 0xB1, 0x45, 0x49, 0x00 },
            new byte[] { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0 });


        /// <summary>
        /// Compare EPC against prefix
        /// </summary>
        /// <param name="epcBytes">EPC, as byte array</param>
        /// <param name="prefix">Prefix to match against EPC</param>
        /// <returns>True if EPC starts with specified prefix</returns>
        private static bool matchesPrefix(byte[] epcBytes, Prefix prefix)
        {
            for (int i = 0; i < prefix.value.Length; i++)
            {
                byte maskByte = prefix.mask[i];
                // TODO: Why does (byte & byte) return int?
                byte refByte = (byte)(prefix.value[i] & maskByte);
                byte valByte = (byte)(epcBytes[i] & maskByte);
                if (refByte != valByte) return false;
            }
            return true;
        }


        // Base36 //////////////////////////////////////////////////////////

        /// <summary>
        /// Encode bytes into base36
        /// </summary>
        /// <param name="bytes">Bytes to encode</param>
        /// <param name="bitCount">Number of bits to encode.
        /// Taken from rightmost end of bytes (least-significant bits)</param>
        /// <returns>base36 string representing input bytes</returns>
        private static string BytesToBase36String(byte[] bytes, int bitCount)
        {
            // Extract least-significant bits from relevant bytes
            int byteCount = BytesPerBits(bitCount);
            long value = 0;
            // Make sure bits will fit into accumulator
            //assert(64 > bitCount);
            // TODO: Figure out the impact of assert in .NET.  For now, be careful.
            // Pull bytes from end
            for (int i = bytes.Length - byteCount; i < bytes.Length; i++)
            {
                value <<= 8;
                value |= bytes[i];
            }
            // Mask off target bits (i.e., clear "extra" bits from leftmost byte)
            long mask = (1L << bitCount) - 1;
            value = value & mask;

            // Encode to base36
            string lowerString = ToBaseNString(value, base36Chars);
            // Pad out to full width
            int minDigits = Base36DigitsPerBits(bitCount);
            if (lowerString.Length < minDigits)
            {
                lowerString = lowerString.PadLeft(minDigits, base36Chars[0]);
            }

            // Reverse digits to maximize variability at beginning of string
            char[] lowerChars = lowerString.ToCharArray();
            System.Array.Reverse(lowerChars);
            lowerString = new string(lowerChars);
            return lowerString;
        }

        /// <summary>
        /// How many base36 digits are necessary to represent a number of bits?
        /// </summary>
        /// <param name="bits">Number of bits to cover</param>
        /// <returns>Number of base36 digits necessary to represent specified number of bits</returns>
        private static int Base36DigitsPerBits(int bits)
        {
            // ceil( log(2**bits) / log(36) ), but logarithms are slow,
            // so we precompute common values.
            switch (bits)
            {
                //case 36:
                //    return 7;
                //case 38:
                //    return 8;
                default:
                    return (int)(Math.Ceiling(Math.Log(Math.Pow(2, bits)) / Math.Log(36)));
            }
        }

        /// <summary>
        /// How many bytes are necessary to contain a number of bits?
        /// </summary>
        /// <param name="bits">Number of bits in question</param>
        /// <returns>Number of bytes required to hold specified number of bits</returns>
        private static int BytesPerBits(int bits)
        {
            if (0 == bits) return 0;
            else return ((bits - 1) / 8) + 1;
        }
        
        
        // Base41 //////////////////////////////////////////////////////////

        /// <summary>
        /// Encode bytes into base41.  Every 2 bytes map into 3 base41 symbols.
        /// </summary>
        /// <param name="bytes">Bytes to encode.  Length must be a multiple of 4.</param>
        /// <returns>base41 string representing input bytes.</returns>
        private static string BytesToBase41String(byte[] bytes)
        {
           string base41String = "";
            // Iterate 2 bytes at a time
            for (int i = 0; i < bytes.Length; i += 2)
            {
                // Pack 2 bytes into one number
                long value = (bytes[i] << 8) + (bytes[i + 1] << 0);
                // Convert number to base41
                string base41Sub = ToBaseNString(value, base41Chars);
                // Pad base41 substring out to 3 digits
                for (int iPad = base41Sub.Length; iPad < 3; iPad++)
                {
                    base41String += base41Chars[0];
                }
                // Accumulate base41 string
                base41String += base41Sub;
            }
            return base41String;
        }

        /// <summary>
        /// Decode base41 into bytes.  Every 3 base41 symbols map to 2 bytes.
        /// </summary>
        /// <param name="base41String">base41 string to decode</param>
        /// <returns>Byte array</returns>
        private static byte[] Base41StringToBytes(string base41String)
        {
            ArrayList byteList = new ArrayList();
            // Iterate 3 base41 symbols at a time
            for (int i = 0; i < base41String.Length; i += 3)
            {
                // Translate base41 to number
                string base41Sub = base41String.Substring(i, 3);
                long value = FromBaseNString(base41Sub, base41Chars);
                // Split number into 2 bytes and accumulate
                byteList.Add((byte)((value >> 8) & 0xFF));
                byteList.Add((byte)((value >> 0) & 0xFF));
            }
            return (byte[])byteList.ToArray(typeof(byte));
        }


        // BaseN //////////////////////////////////////////////////////////
        
        /// <summary>
        /// Convert number to string expressed in an arbitrary base, with arbitrary characters.
        /// e.g.,
        ///   ToBaseNString(0xE, "01") -> "1110"
        ///   ToBaseNString(0xE, "0123456789") -> "14"
        ///   ToBaseNString(0xE, "0123456789abcdef") -> "e"
        /// </summary>
        /// <param name="value">[16-bit] number to convert to string</param>
        /// <param name="basechars">Ordered characters of the target base</param>
        private static string ToBaseNString(long value, string basechars)
        {
            // Compute the base (number of symbols in basechars)
            int n = basechars.Length;
            // Make a place to accumulate digits
            string digitString = "";

            // Iterate until value is exhausted, but don't test up front
            // because we always want to produce at least one digit.
            // Therefore, test for termination at the end of the loop.
            do
            {
                // Current digit is the remainder of value divided by base.
                int digitValue = (int)(value % n);
                // Look up symbol for digit
                char digitSymbol = basechars[digitValue];
                // Accumulate digit (Prepend to get most-significant first; each digit calculated
                // is more significant than the preceding ones.)
                digitString = digitSymbol + digitString;

                // Advance to next digit: Remaining value is dividend of value divided by base.
                // Note: remaining value is thus shifted down by one base digit value.
                value /= n;
            }
            // Stop when there is no more remaining value.
            while (0 < value);

            return digitString;
        }

        /// <summary>
        /// Convert arbitrary-base string expressed to number.
        /// e.g.,
        ///   FromBaseNString("1110", "01") -> 14
        ///   FromBaseNString("14", "0123456789") -> 14
        ///   FromBaseNString("e", "0123456789abcdef") -> 14
        /// </summary>
        /// <param name="value">[16-bit] number to convert to string</param>
        /// <param name="basechars">Ordered characters of the target base</param>
        private static long FromBaseNString(string valueString, string basechars)
        {
            // Compute the base (number of symbols in basechars)
            int n = basechars.Length;
            // Make an accumulator for the value
            long value = 0;

            // Iterate through all digits, least-significant first.
            // Each digit is worth n (base value) times as much as the previous one.
            long digitWeight = 1;
            for (int i = valueString.Length - 1; 0 <= i; i--)
            {
                // Look up digit value (position within basechar set)
                // Weight digit by position (base**digitPosition)
                // Accumulate digit value
                value += basechars.IndexOf(valueString[i]) * digitWeight;
                // Next digit is worth n times as much
                digitWeight *= n;
            }
            return value;
        }

        /// <summary>
        /// Converts Hexadecimal EPC into reversed B36 encoded string
        /// </summary>
        /// <param name="hex"></param>
        /// <returns></returns>
        public static String ConvertHexToBase36(String hex)
        {
            BigInteger big = new BigInteger(hex.ToUpper(), 16);
            StringBuilder sb = new StringBuilder(big.ToString(36));
            char[] charArray = sb.ToString().ToCharArray();
            Array.Reverse(charArray);
            return new string(charArray).ToUpper();
        }

        /// <summary>
        /// Converts B36 encoded string into Hexadecimal EPC
        /// </summary>
        /// <param name="b36"></param>
        /// <returns>Hex string</returns>
        public static String ConvertBase36ToHex(String b36)
        {
            StringBuilder sb = new StringBuilder(b36.ToUpper());
            char[] charArray = sb.ToString().ToCharArray();
            Array.Reverse(charArray);
            BigInteger big = new BigInteger(new string(charArray), 36);
            return big.ToString(16).ToUpper();
        }
    }
}
