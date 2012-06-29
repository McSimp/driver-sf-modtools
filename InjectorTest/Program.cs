using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using Syringe;
using System.Diagnostics;

namespace InjectorTest
{
    class Program
    {
        [StructLayout(LayoutKind.Sequential)]
        struct MessageStruct
        {
            [CustomMarshalAs(CustomUnmanagedType.LPStr)]
            public string Message;
        }
        const string LIB_NAME = "TestDetour.dll";
        static Injector syringe;

        static bool InjectDLL()
        {
            Console.WriteLine("Waiting for process");
            Process driver = null;
            while (driver == null)
            {
                Process[] procs = Process.GetProcessesByName("Driver");
                if (procs.Length != 0)
                {
                    driver = procs[0];
                }
                else
                {
                    Console.Write(".");
                    System.Threading.Thread.Sleep(1000);
                }
            }
            Console.WriteLine();

            syringe = new Injector(driver);
            try
            {
                syringe.InjectLibrary(LIB_NAME);
            }
            catch (Exception e)
            {
                Console.WriteLine("INJECTION ERROR: " + e.Message);
                return false;
            }

            return true;
        }

        static void Main(string[] args)
        {
            Console.WriteLine("Injecting into Driver.exe");
            if(!InjectDLL())
            {
                Console.WriteLine("Press any key to exit");
                Console.ReadLine();
                return;
            }

            Console.WriteLine("Ready to execute Lua");
            while (true)
            {
                string command = Console.ReadLine();
                MessageStruct messageData = new MessageStruct() { Message = command };
                syringe.CallExport(LIB_NAME, "ExecuteLua", messageData);
            }
        }
    }
}
