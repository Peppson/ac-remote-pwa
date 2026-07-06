using System;
using System.Collections.Generic;
using System.Diagnostics;

class ChecksumCracker
{
    const string temp_32 =     "11000001 01100000 11000000 01000111 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01000011 0";
    const string temp_31 =     "11000001 01100000 11000000 01001011 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01000101 0";
    const string temp_30 =     "11000001 01100000 11000000 01000011 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01000001 0";
    const string temp_29 =     "11000001 01100000 11000000 01001101 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01000110 0";
    const string temp_24 =     "11000001 01100000 11000000 01000110 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01000011 1";
    const string temp_21 =     "11000001 01100000 11000000 01001100 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01000110 1";
    const string temp_20 =     "11000001 01100000 11000000 01000100 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01000010 1";
    const string temp_18 =     "11000001 01100000 11000000 01000000 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01000000 1";

    const string jet_off =     "11000001 01100000 11000000 01000111 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01000011 0";
    const string jet_onn =     "11000001 01100000 10000000 01000000 00000000 00001001 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01100100 0";

    const string mode_cool =   "11000001 01100000 00000000 01000001 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 00100000 0";
    const string mode_dry  =   "11000001 01100000 00000000 11001110 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01100111 1";
    const string mode_fan  =   "11000001 01100000 10000000 00101110 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01010111 1";

    const string temp_32a =    "11000001 01100000 00000000 01000111 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 00100011 0";
    const string lid_toggle =  "11000001 01100000 00000001 01000111 00000000 00000000 10000000 10000000 00000001 00000000 00000000 00000000 00000000 00100010 1";

    const string fan_high =    "11000001 01100000 10000000 01000111 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01100011 0";
    const string fan_med =     "11000001 01100000 01000000 01000111 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 00000011 0";
    const string fan_low =     "11000001 01100000 11000000 01000111 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01000011 0";

    const string tim_0h =      "11000001 01100000 01000000 00101110 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 00110111 1";
    const string tim_1h =      "11000001 01100000 01000000 00111110 01000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 00011111 1";
    const string tim_5h =      "11000001 01100000 01000000 00111110 01010000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 00010111 1";

    const string auto_on =     "11000001 01100000 00000000 10001110 00000001 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01000111 0";
    const string auto_off =    "11000001 01100000 01000000 00101110 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 00110111 1";

    const string around_on =   "11000001 01100000 01000000 00101110 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 11001100 01111011 0";
    const string around_off =  "11000001 01100000 01000000 00101110 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 00110111 1";

    const string temp_32b =    "11000001 01100000 00000000 01000111 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 00100011 0";
    const string leds_off =    "11000001 01100000 00000000 01000111 00000000 00000000 10000010 10000000 00000000 00000000 00000000 00000000 00000000 00100001 0";

    const string sleep_off =   "11000001 01100000 00000000 01000111 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 00100011 0";
    const string sleep_on =    "11000001 01100000 11010000 01000111 00000000 00000000 10000000 10000000 00000000 00000000 00000000 00000000 00000000 01001011 0";

    struct FrameSample
    {
        public string name;
        public string bits;
        public byte[] frame;
        public byte tail;
    }

    enum MapMode : byte
    {
        MAP_RAW8 = 0,
        MAP_REV8 = 1,
        MAP_INV8 = 2,
        MAP_INVREV8 = 3,
        MAP_KEEP_HIGH_USE_LOW = 4,
        MAP_40_LOW = 5
    }

    struct Candidate
    {
        public int score;
        public int uniquePred;
        public byte width;
        public ushort poly;
        public ushort init;
        public ushort xorout;
        public bool refin;
        public bool refout;
        public bool reverseBytes;
        public bool invertBytes;
        public bool includeTail;
        public MapMode mapMode;
    }

    static bool ParseFrame14(string s, out byte[] frame, out byte tail)
    {
        frame = new byte[14];
        tail = 255;
        int bitCount = 0, cur = 0, outIndex = 0;

        foreach (char c in s)
        {
            if (c != '0' && c != '1') continue;
            int b = c - '0';

            if (outIndex < 14)
            {
                cur = (cur << 1) | b;
                bitCount++;
                if (bitCount == 8)
                {
                    frame[outIndex++] = (byte)cur;
                    cur = 0;
                    bitCount = 0;
                }
            }
            else
            {
                tail = (byte)b;
                break;
            }
        }

        return outIndex == 14 && bitCount == 0 && tail <= 1;
    }

    static byte Reverse8(byte v)
    {
        v = (byte)(((v & 0xF0) >> 4) | ((v & 0x0F) << 4));
        v = (byte)(((v & 0xCC) >> 2) | ((v & 0x33) << 2));
        v = (byte)(((v & 0xAA) >> 1) | ((v & 0x55) << 1));
        return v;
    }

    static ushort ReflectN(ushort x, byte width)
    {
        ushort r = 0;
        for (int i = 0; i < width; i++)
            if ((x & (1u << i)) != 0) r |= (ushort)(1u << (width - 1 - i));
        return r;
    }

    static ushort CrcGeneric(byte[] frame, byte tail, byte width, ushort poly, ushort init, ushort xorout,
        bool refin, bool refout, bool reverseBytes, bool invertBytes, bool includeTail)
    {
        ushort mask = (ushort)((1u << width) - 1u);
        ushort crc = (ushort)(init & mask);

        for (int i = 0; i < 13; i++)
        {
            byte b = frame[i];
            if (reverseBytes) b = Reverse8(b);
            if (invertBytes) b = (byte)~b;

            if (refin)
            {
                for (int k = 0; k < 8; k++)
                {
                    byte bit = (byte)((b >> k) & 1u);
                    byte fb = (byte)((crc & 1u) ^ bit);
                    crc >>= 1;
                    if (fb != 0) crc ^= poly;
                    crc &= mask;
                }
            }
            else
            {
                for (int k = 0; k < 8; k++)
                {
                    byte bit = (byte)((b >> (7 - k)) & 1u);
                    byte fb = (byte)(((crc >> (width - 1)) & 1u) ^ bit);
                    crc = (ushort)((crc << 1) & mask);
                    if (fb != 0) crc ^= poly;
                    crc &= mask;
                }
            }
        }

        if (includeTail)
        {
            byte bit = (byte)(tail & 1u);
            if (refin)
            {
                byte fb = (byte)((crc & 1u) ^ bit);
                crc >>= 1;
                if (fb != 0) crc ^= poly;
                crc &= mask;
            }
            else
            {
                byte fb = (byte)(((crc >> (width - 1)) & 1u) ^ bit);
                crc = (ushort)((crc << 1) & mask);
                if (fb != 0) crc ^= poly;
                crc &= mask;
            }
        }

        if (refout) crc = ReflectN(crc, width);
        crc ^= xorout;
        crc &= mask;
        return crc;
    }

    static byte MapPrediction(byte got, byte calc8, MapMode m)
    {
        return m switch
        {
            MapMode.MAP_RAW8 => calc8,
            MapMode.MAP_REV8 => Reverse8(calc8),
            MapMode.MAP_INV8 => (byte)~calc8,
            MapMode.MAP_INVREV8 => Reverse8((byte)~calc8),
            MapMode.MAP_KEEP_HIGH_USE_LOW => (byte)((got & 0xF0) | (calc8 & 0x0F)),
            MapMode.MAP_40_LOW => (byte)(0x40 | (calc8 & 0x0F)),
            _ => calc8
        };
    }

    static string MapName(MapMode m) => m switch
    {
        MapMode.MAP_RAW8 => "RAW8",
        MapMode.MAP_REV8 => "REV8",
        MapMode.MAP_INV8 => "INV8",
        MapMode.MAP_INVREV8 => "INVREV8",
        MapMode.MAP_KEEP_HIGH_USE_LOW => "KEEP_HIGH_USE_LOW",
        MapMode.MAP_40_LOW => "0x40_LOW",
        _ => "?"
    };

    static void PushTop(Candidate[] top, Candidate c)
    {
        int pos = -1;
        for (int i = 0; i < 10; i++)
        {
            if (c.score > top[i].score || (c.score == top[i].score && c.uniquePred > top[i].uniquePred))
            {
                pos = i;
                break;
            }
        }
        if (pos < 0) return;
        for (int i = 9; i > pos; i--) top[i] = top[i - 1];
        top[pos] = c;
    }

    static void Main()
    {
        var samples = new (string name, string bits)[]
        {
            ("temp_32", temp_32), ("temp_31", temp_31), ("temp_30", temp_30), ("temp_29", temp_29),
            ("temp_24", temp_24), ("temp_21", temp_21), ("temp_20", temp_20), ("temp_18", temp_18),
            ("jet_off", jet_off), ("jet_onn", jet_onn),
            ("mode_cool", mode_cool), ("mode_dry", mode_dry), ("mode_fan", mode_fan),
            ("temp_32a", temp_32a), ("lid_toggle", lid_toggle),
            ("fan_high", fan_high), ("fan_med", fan_med), ("fan_low", fan_low),
            ("tim_0h", tim_0h), ("tim_1h", tim_1h), ("tim_5h", tim_5h),
            ("auto_on", auto_on), ("auto_off", auto_off),
            ("around_on", around_on), ("around_off", around_off),
            ("temp_32b", temp_32b), ("leds_off", leds_off),
            ("sleep_off", sleep_off), ("sleep_on", sleep_on)
        };

        // Pre-parse all samples
        var parsed = new List<FrameSample>();
        foreach (var (name, bits) in samples)
        {
            if (ParseFrame14(bits, out byte[] frame, out byte tail))
                parsed.Add(new FrameSample { name = name, bits = bits, frame = frame, tail = tail });
        }

        Console.WriteLine($"Pre-parsed {parsed.Count} samples");

        var top = new Candidate[10];
        for (int i = 0; i < 10; i++) top[i].score = -1;

        ushort[] initSet8 = { 0x00, 0xFF, 0x55, 0xAA, 0x12, 0xD2 };
        ushort[] xorSet8 = { 0x00, 0xFF, 0x55, 0xAA, 0x12, 0xD2 };
        ushort[] initSet4 = { 0x0, 0x1, 0x3, 0x5, 0x7, 0xF };
        ushort[] xorSet4 = { 0x0, 0x1, 0x3, 0x5, 0x7, 0xF };

        var sw = Stopwatch.StartNew();
        int totalTested = 0;

        for (int widthIdx = 0; widthIdx < 2; widthIdx++)
        {
            byte width = (byte)(widthIdx == 0 ? 4 : 8);
            ushort polyMax = (byte)(width == 4 ? 0x0F : 0xFF);

            for (ushort poly = 1; poly <= polyMax; poly++)
            {
                for (int refin = 0; refin < 2; refin++)
                {
                    for (int refout = 0; refout < 2; refout++)
                    {
                        for (int revBytes = 0; revBytes < 2; revBytes++)
                        {
                            for (int invBytes = 0; invBytes < 2; invBytes++)
                            {
                                for (int incTail = 0; incTail < 2; incTail++)
                                {
                                    for (int map = 0; map < 6; map++)
                                    {
                                        var initSet = width == 4 ? initSet4 : initSet8;
                                        var xorSet = width == 4 ? xorSet4 : xorSet8;

                                        foreach (var init in initSet)
                                        {
                                            foreach (var xorval in xorSet)
                                            {
                                                int score = 0;
                                                var seen = new bool[256];
                                                int uniquePred = 0;

                                                foreach (var sample in parsed)
                                                {
                                                    byte got = sample.frame[13];
                                                    ushort crc = CrcGeneric(sample.frame, sample.tail, width, poly,
                                                        init, xorval, refin != 0, refout != 0, revBytes != 0,
                                                        invBytes != 0, incTail != 0);

                                                    byte calc8 = width == 4 ? (byte)(crc & 0x0F) : (byte)(crc & 0xFF);
                                                    byte pred = MapPrediction(got, calc8, (MapMode)map);

                                                    if (!seen[pred])
                                                    {
                                                        seen[pred] = true;
                                                        uniquePred++;
                                                    }

                                                    if (pred == got) score++;
                                                }

                                                if (uniquePred < 4) continue;

                                                var c = new Candidate
                                                {
                                                    score = score,
                                                    uniquePred = uniquePred,
                                                    width = width,
                                                    poly = poly,
                                                    init = init,
                                                    xorout = xorval,
                                                    refin = refin != 0,
                                                    refout = refout != 0,
                                                    reverseBytes = revBytes != 0,
                                                    invertBytes = invBytes != 0,
                                                    includeTail = incTail != 0,
                                                    mapMode = (MapMode)map
                                                };
                                                PushTop(top, c);
                                                totalTested++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        sw.Stop();
        Console.WriteLine($"\n=== TOP CANDIDATES (ALL SAMPLES) ===");
        Console.WriteLine($"Tested {totalTested} candidates in {sw.ElapsedMilliseconds}ms\n");

        for (int i = 0; i < 10; i++)
        {
            if (top[i].score < 0) continue;
            Console.WriteLine($"#{i + 1} score={top[i].score} unique={top[i].uniquePred} width={top[i].width} poly=0x{top[i].poly:X} init=0x{top[i].init:X} xor=0x{top[i].xorout:X} refin={(top[i].refin ? 1 : 0)} refout={(top[i].refout ? 1 : 0)} revBytes={( top[i].reverseBytes ? 1 : 0)} invBytes={( top[i].invertBytes ? 1 : 0)} tail={( top[i].includeTail ? 1 : 0)} map={MapName(top[i].mapMode)}");
        }

        Console.WriteLine("\nDone!");
    }
}