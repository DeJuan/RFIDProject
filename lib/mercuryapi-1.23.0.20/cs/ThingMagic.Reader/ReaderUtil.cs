using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Threading;

namespace ThingMagic
{
    class ReaderUtil 
    {
        //static Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        public static void FirmwareLoadUtil(Reader reader, Stream firmware, FixedReaderFirmwareLoadOptions rflOptions, string hostName, ref Socket socket)
        {
            // Assume that a system with an RQL interpreter has the standard
            // web interface and password. This isn't really an RQL operation,
            // but it will work most of the time
            HttpWebRequest req = null;// = MakeWebReq("/cgi-bin/firmware.cgi");
            string uri = (string)reader.ParamGet("/reader/uri");

            // Filename "firmware.tmfw" is arbitrary -- server always ignores
            // Fixed reader firmware is huge (~7MB).
            // Increase the default timeout to allow sufficient upload time
            // (Default timeout is 100 seconds)
            if (null == rflOptions)
            {
                req = MakeWebReq("/cgi-bin/firmware.cgi",hostName);
            }
            else
            {
                FixedReaderFirmwareLoadOptions objrflOptions;
                objrflOptions = (FixedReaderFirmwareLoadOptions)rflOptions;
                if ((!objrflOptions.EraseContents) && (!objrflOptions.RevertDefaultSettings))
                {
                    req = MakeWebReq("/cgi-bin/firmware.cgi", hostName);
                }
                else if ((!objrflOptions.EraseContents) && (objrflOptions.RevertDefaultSettings))
                {
                    throw new ReaderException("Un supported option type");
                }
                else if (objrflOptions.EraseContents && objrflOptions.RevertDefaultSettings)
                {
                    req = MakeWebReq("/cgi-bin/firmware.cgi?revert=true&wipe=true&DOWNGRADE=Continue", hostName);
                }
                else if ((objrflOptions.EraseContents) && (!objrflOptions.RevertDefaultSettings))
                {
                    req = MakeWebReq("/cgi-bin/firmware.cgi?wipe=true&DOWNGRADE=Continue", hostName);
                }
            }
            req.Timeout = 10 * 60 * 1000;
            WebUploadFile(req, firmware, "firmware.tmfw");

            HttpWebResponse rsp = (HttpWebResponse)req.GetResponse();
            string response = WebRespToString(rsp);

            if ((0 <= response.IndexOf("Firmware file uploaded")) || (0 <= response.IndexOf("Press Continue to install")))
            {
                WebRespToString((HttpWebResponse)MakeWebReq("/cgi-bin/firmware.cgi?confirm=true", hostName
                    ).GetResponse());
            }
            else if ((0 <= response.IndexOf("replace the new firmware with older firmware")))
            {
                // If asked to confirm using an older firmware, respond
                WebRespToString((HttpWebResponse)MakeWebReq("/cgi-bin/firmware.cgi?wipe=true&confirm=true&DOWNGRADE=Continue", hostName
                    ).GetResponse());
            }

            // If firmware load succeeded, reboot to make it take effect
            if ((0 <= response.IndexOf("Firmware update complete")) || (0 <= response.IndexOf("Firmware upgrade started")))
            {
                // Restart reader
                HttpWebRequest rebootReq = MakeWebReq("/cgi-bin/reset.cgi",hostName);
                WebPost(rebootReq, "dummy=dummy");
                // TODO: Use a more sophisticated method to detect when the reader is ready again
                System.Threading.Thread.Sleep(90 * 1000);
            
           }
            else
                throw new ReaderException("Firmware update failed");
        }

        #region HTTP Post Methods

        #region MakeWebReq

        /// <summary>
        /// Create web request, using readers' default username and password
        /// </summary>
        /// <param name="uripath"></param>
        /// <param name="hostName"></param>
        /// <returns></returns>
        private static HttpWebRequest MakeWebReq(string uripath, string hostName)
        {
            string url = String.Format("http://{0}{1}", hostName, uripath);
            HttpWebRequest req = (HttpWebRequest)WebRequest.Create(url);
            byte[] authBytes = Encoding.UTF8.GetBytes("web:radio".ToCharArray());
            req.Headers["Authorization"] = "Basic " + Convert.ToBase64String(authBytes);

            return req;
        }

        #endregion

        #region ReadAllBytes

        private static byte[] ReadAllBytes(string filename)
        {
            return ReadAllBytes(new StreamReader(filename).BaseStream);
        }

        private static byte[] ReadAllBytes(Stream stream)
        {
            BinaryReader br = new BinaryReader(stream);
            List<byte> list = new List<byte>();

            while (true)
            {
                byte[] chunk = br.ReadBytes(1024 * 1024);

                if (0 == chunk.Length)
                    break;

                list.AddRange(chunk);
            }

            return list.ToArray();
        }

        #endregion

        #region ShowBody

        private static void ShowBody(HttpWebRequest req, string body)
        {
            Console.WriteLine(String.Format("Read {0:D} bytes from {1}", body.Length, req.RequestUri));
            Console.WriteLine("First KB:");
            Console.WriteLine(body.Substring(0, 1024));
        }

        #endregion

        #region WebPost

        private static void WebPost(HttpWebRequest req, string content)
        {
            WebPost(req, "application/x-www-form-urlencoded", Encoding.UTF8.GetBytes(content));
        }

        private static void WebPost(HttpWebRequest req, string contentType, string content)
        {
            WebPost(req, contentType, Encoding.UTF8.GetBytes(content));
        }

        private static void WebPost(HttpWebRequest req, string contentType, byte[] reqbody)
        {
            req.Method = "POST";
            req.ContentType = contentType;
            req.ContentLength = reqbody.Length;
            Stream reqStream = req.GetRequestStream();
            reqStream.Write(reqbody, 0, reqbody.Length);
            reqStream.Close();
        }

        #endregion

        #region WebRespToReader

        private static StreamReader WebRespToReader(HttpWebResponse rsp)
        {
            return new StreamReader(rsp.GetResponseStream());
        }

        #endregion

        #region WebRespToString

        private static string WebRespToString(HttpWebResponse rsp)
        {
            return WebRespToReader(rsp).ReadToEnd();
        }

        #endregion

        #region WebUploadFile

        private static void WebUploadFile(HttpWebRequest req, Stream contentStream, string filename)
        {
            string boundary = "MercuryAPIFormBoundary9ef3cac3aeca9440ba8a55224a363a0d";
            string contentType = String.Format("multipart/form-data; boundary={0}", boundary);
            List<byte> content = new List<byte>();
            content.AddRange(Encoding.ASCII.GetBytes(String.Format(String.Join("\r\n", new string[] {
                "--{0}",
                "Content-Disposition: form-data; name=\"uploadfile\"; filename=\"{1}\"",
                "\r\n",
            }), boundary, filename)));
            content.AddRange(ReadAllBytes(contentStream));
            content.AddRange(Encoding.ASCII.GetBytes(String.Format(
                "\r\n--{0}--\r\n",
                boundary)));
            WebPost(req, contentType, content.ToArray());
        }

        #endregion

        #endregion
    }
}