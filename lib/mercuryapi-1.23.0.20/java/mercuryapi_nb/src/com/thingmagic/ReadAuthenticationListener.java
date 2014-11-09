/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package com.thingmagic;

/**
 *
 * @author qvantel
 */
public interface ReadAuthenticationListener {
 /**
   * Invoked when a tag read Authentication exception occurs
   *
   * @param r the Reader where the tag was read
   * @param t the tag data and metadata
   */
  void readTagAuthentication(TagReadData t,Reader reader)throws ReaderException;
}
