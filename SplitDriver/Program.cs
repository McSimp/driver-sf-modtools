using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Text;
using System.Diagnostics;

namespace SplitDriver
{
    class Program
    {
        static void Main(string[] args)
        {
            byte[] pattern = { 0x1B, 0x4C, 0x75, 0x61, 0x51 };
            FileStream fs = new FileStream("ScriptsCore.fchunk", FileMode.Open);

            int numFiles = 0;

            long offset = ReadTo(pattern, fs);
            while (offset >= 0)
            {
                numFiles++;
                string filepath = GetFilename(fs);
                Console.WriteLine(filepath);
                
                long nextOffset = ReadTo(pattern, fs);
                if (nextOffset > offset)
                {
                    long fileLength = nextOffset - offset;
                    byte[] data = new byte[fileLength];
                    fs.Seek(offset, SeekOrigin.Begin);
                    fs.Read(data, 0, (int)fileLength);
                    fs.Seek(pattern.Length, SeekOrigin.Current);
                    string cpath = "compiled\\" + filepath + "c";
                    string dpath = "decompiled\\" + filepath;
                    Directory.CreateDirectory(Path.GetDirectoryName(cpath));
                    Directory.CreateDirectory(Path.GetDirectoryName(dpath));
                    File.WriteAllBytes(cpath, data);

                    Process p = new Process();
                    p.StartInfo.FileName = "Luadec51.exe";
                    p.StartInfo.UseShellExecute = false;
                    p.StartInfo.RedirectStandardOutput = true;
                    p.StartInfo.CreateNoWindow = false;
                    p.StartInfo.Arguments = cpath;
                    p.Start();

                    StreamReader sr = p.StandardOutput;
                    while (!sr.EndOfStream)
                    {
                        String output = sr.ReadToEnd();
                        File.WriteAllBytes(dpath, Encoding.ASCII.GetBytes(output));
                    }
                }

                offset = nextOffset; 
            }

            Console.WriteLine(numFiles);
            Console.ReadLine();
        }

        static string GetFilename(Stream stream)
        {
            stream.Seek(44, SeekOrigin.Current);

            StringBuilder sb = new StringBuilder();
            int b = stream.ReadByte();
            while (b > 0)
            {
                sb.Append((char)b);
                b = stream.ReadByte();
            }

            return sb.ToString();
        }
        
        static long ReadTo(byte[] pattern, Stream stream)
        {
            //I will have to implement some sort of linear string matching algorithm.
            long startPosition = stream.Position;
            //Knuth-Morris-Pratt algorithm:
            int[] f = kmpFailureFunction(pattern);
            //int i=0;
            int j = 0;
            int tmp = stream.ReadByte();
            if (tmp < 0)
                return -1;//end of stream
            byte t = (byte)tmp;
            while (stream.Position < stream.Length + 1)
            {
                if (pattern[j] == t)
                {
                    if (j == pattern.Length - 1)//see if we have a match
                        return stream.Position - pattern.Length;//i-m+1
                    //i++;//do I need this?
                    tmp = stream.ReadByte();
                    if (tmp < 0)
                        return -1;//end of stream
                    t = (byte)tmp;
                    j++;
                }
                else if (j > 0)
                {//move forward in the pattern
                    j = f[j - 1];
                }
                else
                {
                    //i++;
                    tmp = stream.ReadByte();
                    if (tmp < 0)
                        return -1;//end of stream
                    t = (byte)tmp;
                }
            }
            return -1;//There is no matching substring
        }

        static int[] kmpFailureFunction(byte[] pattern)
        {
            int i = 1;
            int j = 0;
            int[] f = new int[pattern.Length];
            if (f.Length > 0)
                f[0] = 0;
            while (i < pattern.Length)
            {
                if (pattern[j] == pattern[i])
                {
                    //we have matched j+1 characters
                    f[i] = j + 1;
                    i++;
                    j++;
                }
                else if (j > 0)
                {
                    //j indexes just after a prefic of P that must match
                    j = f[j - 1];
                }
                else
                {
                    //no match
                    f[i] = 0;
                    i++;
                }
            }
            return f;
        }


    }
}
