document.addEventListener('DOMContentLoaded', function(event) {
    var math = document.querySelectorAll('span.math');
    for(var i = 0; i < math.length; ++i)
    {
        if(math[i].classList.contains('displaystyle'))
            katex.render('\\displaystyle { ' + math[i].textContent + ' }', math[i]);
        else
            katex.render(math[i].textContent, math[i]);
    }
});
