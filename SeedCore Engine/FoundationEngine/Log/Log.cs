using System;
using System.Text;

namespace SeedCore
{
    public static unsafe class Log
    {
        private enum Level : Byte
        {
            Notice,
            Warning,
            Error,
        }

        public static void Notice(String message)
        {
            Send(Level.Notice, message);
        }

        public static void Warning(String message)
        {
            Send(Level.Warning, message);
        }

        public static void Error(String message)
        {
            Send(Level.Error, message);
        }

        private static void Send(Level level, String message)
        {
            CsharpNativeApi* api = CsharpNativeApi.current_;
            if(api==null)
            {
                return;
            }

            Byte[] bytes = Encoding.UTF8.GetBytes(message);
            fixed(Byte* text=bytes)
            {
                api->log_((Byte)level, text, bytes.Length);
            }
        }
    }
}
