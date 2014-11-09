using System;
using System.Collections.Generic;
using System.ComponentModel;

namespace ThingMagic.URA2
{
    // Based on:
    // http://stackoverflow.com/questions/249779/datagridview-sort-and-e-g-bindinglistt-in-net
    // http://msdn.microsoft.com/en-us/library/ms993236.aspx
    // http://stackoverflow.com/questions/1063917/bindinglistt-sort-to-behave-like-a-listt-sort

    /// <summary>
    /// A BindingList that supports sorting.
    /// 
    /// Sample Usage:
    /// 
    /// class MyBindingList : ThingMagic.SortableBindingList<Datum>
    /// {
    ///     protected override Comparison<Datum> GetComparer(PropertyDescriptor prop)
    ///     {
    ///         Comparison<Datum> comparer = null;
    ///         switch (prop.Name)
    ///         {
    ///             case "Name":
    ///                 comparer = new Comparison<Datum>(delegate(Datum a, Datum b)
    ///                 {
    ///                     return String.Compare(a.Name, b.Name);
    ///                 });
    ///                 break;
    ///         }
    ///         return comparer;
    ///     }
    /// }
    /// </summary>
    /// <typeparam name="T">Type of list items</typeparam>
    public class SortableBindingList<T> : BindingList<T>
    {
        protected override bool SupportsSortingCore
        {
            get
            {
                return true;
            }
        }

        protected override bool IsSortedCore
        {
            get
            {
                return _isSorted;
            }
        }
        private bool _isSorted = false;

        protected override PropertyDescriptor SortPropertyCore
        {
            get
            {
                return _sortProperty;
            }
        }
        private PropertyDescriptor _sortProperty;

        protected override ListSortDirection SortDirectionCore
        {
            get
            {
                return _sortDirection;
            }
        }
        private ListSortDirection _sortDirection;

        protected override void ApplySortCore(PropertyDescriptor prop, ListSortDirection direction)
        {
            if (null != prop.PropertyType.GetInterface("IComparable"))
            {
                List<T> itemsList = (List<T>)this.Items;
                Comparison<T> comparer = GetComparer(prop);
                itemsList.Sort(comparer);
                if (direction == ListSortDirection.Descending)
                {
                    itemsList.Reverse();
                }
                _isSorted = true;
                _sortProperty = prop;
                _sortDirection = direction;
                this.OnListChanged(new ListChangedEventArgs(ListChangedType.Reset, -1));
            }
        }

        protected virtual Comparison<T> GetComparer(PropertyDescriptor prop) 
        {
            throw new NotImplementedException();
        }

        protected override void RemoveSortCore() { }

        protected override void OnListChanged(ListChangedEventArgs e)
        {
            if (null != SortPropertyCore)
            {
                if (!_insideListChangedHandler)
                {
                    _insideListChangedHandler = true;
                    ApplySortCore(SortPropertyCore, SortDirectionCore);
                    _insideListChangedHandler = false;
                }
            }
            base.OnListChanged(e);
        }
        private bool _insideListChangedHandler = false;
    }
}
