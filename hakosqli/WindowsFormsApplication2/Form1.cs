using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;


using System.Runtime.InteropServices;

namespace WindowsFormsApplication2
{
    public partial class Form1 : Form
    {
        
        [DllImport("DB.dll")]
        private extern static int db_open();
        [DllImport("DB.dll")]
        private extern static int db_close();
        [DllImport("DB.dll")]
        private extern static int db_exec(byte [] sql, byte [] outdata);
        

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            
        }

        private void webBrowser1_LocationChanged(object sender, EventArgs e)
        {
            
        }

        private string html(string addd, string val)
        {
            String s = "<html>" + "<body><h3>箱庭SQLi</3><p>Please input your Number.</p>";
            s += "<form action=\"http://07c00.com/white.html\" method=\"get\">";
            s += "<p>";
            s += "Number:<input type=\"text\" name=\"q\" value=\"" + val + "\" size=\"20\">　";
            s += "<input type=\"submit\" value=\"submit\">";
            s += "</p>";
            s += "</form>";
            s += addd;
            s += "</body>";
            s += "</html>";
            return s;
        }

        private void webBrowser1_Navigated(object sender, WebBrowserNavigatedEventArgs e)
        {
            string ww = "";
            string qq = "";
            string ur = webBrowser1.Url.Query;
            if (ur.Length <= 3)
            {
                return;
            }
            if (ur[0] == '?' && ur[1] == 'q' && ur[2] == '=')
            {
                db_open();

                string aa = ur.Substring(3, ur.Length - 3);
                aa = Uri.UnescapeDataString(aa.Replace('+', ' '));

                string xx = "";
                xx += "CREATE TABLE COMPANY(ID INT PRIMARY KEY NOT NULL, NAME TEXT NOT NULL,AGE INT NOT NULL, ADDRESS CHAR(50), SALARY REAL );";
                xx += "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) VALUES (00000000, 'Tanaka',54, 'Tokyo',         90000 ); ";
                xx += "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) VALUES (00000001, 'Paul',  32, 'California',    20000 ); ";
                xx += "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) VALUES (00000002, 'Allen', 25, 'Texas',         15000 ); ";
                xx += "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) VALUES (00000003, 'Teddy', 23, 'Norway',        20000 ); ";
                xx += "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) VALUES (00000004, 'Mark',  25, 'Rich-Mond',     65000 ); ";
                
                xx += "CREATE TABLE SECCON(FLAG TEXT NOT NULL);";

                xx += "INSERT INTO SECCON (FLAG) VALUES ('FLAG{EnjoySQLi}'); ";
                //MessageBox.Show(aa);
               // ww = "select * from COMPANY where ID='" + aa + "';";
                xx += "SELECT * from COMPANY WHERE ID='" + aa + "'";
                string cc = aa;
                Char[] values = xx.ToCharArray();
                Encoding ascii = Encoding.ASCII;
                Byte[] kk = ascii.GetBytes(values);
                byte[] oo = new byte[9999];
                db_exec(kk, oo);
                qq = "";
                for (int i = 0; oo[i] != 0x00; i++)
                    qq += ((char)oo[i]).ToString();
                if (qq == "")
                {
                    qq = "Error: " + aa;
                }
                //MessageBox.Show(qq);
                webBrowser1.DocumentText = html(qq, cc);
                //
            }
            
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            
        }

        private void Form1_Shown(object sender, EventArgs e)
        {
            

            String s = html("", "00000000");

            webBrowser1.DocumentText = s;
        }

        private void webBrowser1_DocumentCompleted(object sender, WebBrowserDocumentCompletedEventArgs e)
        {

        }
    }
}
