#---
# Excerpted from "Programming Your Home",
# published by The Pragmatic Bookshelf.
# Copyrights apply to this code. It may not be used to create training material, 
# courses, books, articles, and the like. Contact us if you are in doubt.
# We make no guarantees that this code is fit for any purpose. 
# Visit http://www.pragmaticprogrammer.com/titles/mrhome for more book information.
#---
class CommandController < ApplicationController
  def cmd
    @result = params[:cmd]

    if @result == "on"
      %x[/usr/local/bin/heyu on h3]
    end

    if @result == "off"
      %x[/usr/local/bin/heyu off h3]
    end
  end
end
