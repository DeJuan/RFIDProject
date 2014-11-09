using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ThingMagic;
using ThingMagic.URA2.BL;

namespace ThingMagic.URA2.ViewModel
{
    class TagResultsViewModel
    {


        public void ReadTags(ref TagDatabase tagdb, Reader objReader, int readTimeOut, out TagReadData[] trd)
        {
            //fontSize_TextChanged_1(sender, e);
            //Change the datagrid data font size
            //dataGrid1.FontSize = Convert.ToDouble(txtfontSize.Text);
            //GUIturnoffWarning();
            //simpleReadPlans.Clear();
            try
            {
                //FontFamily fFamily = new FontFamily("Arial");
                //dataGrid1.ColumnHeaderStyle 
                //Font objFont = new Font(fFamily, Convert.ToInt64(fontSize.Text));
                //tagResultsGrid.Font = objFont;

                DateTime timeBeforeRead = DateTime.Now;
                TagReadData[] tagID = objReader.Read(readTimeOut);
                DateTime timeAfterRead = DateTime.Now;
                TimeSpan timeElapsed = timeAfterRead - timeBeforeRead;

                trd = tagID;
                tagdb.AddRange(tagID);                
                
            }
            catch (Exception ex)
            {
                throw ex;
            }            
            
        }

    }
}
